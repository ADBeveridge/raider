#include <gtk/gtk.h>
#include "raider-window.h"
#include "raider-file-row.h"

struct _RaiderWindow
{
    GtkApplicationWindow parent;

    GtkWidget *header_bar;
    GtkWidget *contents_box;
    GtkWidget *shred_button;
    GtkWidget *primary_menu;
    GtkWidget *list_box;
    GtkWidget *hide_shredding_check_button;
    GtkWidget *number_of_passes_spin_button;
    GtkWidget *remove_file_check_button;
    GtkWidget *sample;
};

G_DEFINE_TYPE (RaiderWindow, raider_window, GTK_TYPE_APPLICATION_WINDOW)

static void
raider_window_init (RaiderWindow *win)
{
    GtkBuilder *builder;
    GMenuModel *menu;

    gtk_widget_init_template(GTK_WIDGET(win));

    builder = gtk_builder_new_from_resource ("/org/gnome/raider/ui/gears-menu.ui");
    menu = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
    gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (win->primary_menu), menu);
    g_object_unref (builder);

    /* Make the window a DND destination. */
    static GtkTargetEntry targetentries[] = {{ "text/uri-list", 0, 0 }};
    gtk_drag_dest_set (GTK_WIDGET(win), GTK_DEST_DEFAULT_ALL, targetentries, 1, GDK_ACTION_COPY); /* Make it into a dnd destination. */
    g_signal_connect (win, "drag_data_received", G_CALLBACK (on_drag_data_received), win);

    /* Make the shred button visually destructive. */
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, "#shred_button { background-color: #f4534d}", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static void
raider_window_dispose (GObject *object)
{
    G_OBJECT_CLASS (raider_window_parent_class)->dispose (object);
}

static void
raider_window_class_init (RaiderWindowClass *class)
{
    G_OBJECT_CLASS (class)->dispose = raider_window_dispose;

    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/org/gnome/raider/ui/raider-window.ui");

    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, header_bar);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, primary_menu);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, shred_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, contents_box);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, list_box);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), shred_file);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), raider_window_open_file_dialog);
}

/*
 * This is called when the user drops files into the list box.
 */
void on_drag_data_received (GtkWidget *wgt, GdkDragContext *context, gint x, gint y,
                            GtkSelectionData *seldata, guint info, guint time,
                            gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    gchar **filenames = filenames = g_uri_list_extract_uris ( (const gchar *) gtk_selection_data_get_data (seldata) );

    if (filenames == NULL)
    {
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window->header_bar), "Unable to complete Drag and Drop!");

        gtk_drag_finish (context, FALSE, FALSE, time);
        return;
    }

    int iter = 0;

    while (filenames[iter] != NULL)
    {
        char *filename = g_filename_from_uri (filenames[iter], NULL, NULL);
        raider_window_open(filename, data);

        iter++;
    }

    gtk_drag_finish (context, TRUE, FALSE, time);
}

void
raider_window_open (gchar *filename_to_open, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    GFile *file = g_file_new_for_path (filename_to_open);
    if (g_file_query_exists (file, NULL) == FALSE)
    {
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window->header_bar), "File does not exist!");
        return;
    }
    g_object_unref(file);


    GtkWidget *file_row = GTK_WIDGET(raider_file_row_new(filename_to_open));
    gtk_container_add(GTK_CONTAINER (window->list_box), file_row);

    g_free (filename_to_open);
}

void
raider_window_open_file_dialog (GtkWidget *button, RaiderWindow *window)
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new ("Open File", GTK_WINDOW (window),
                        GTK_FILE_CHOOSER_ACTION_OPEN, "Cancel", GTK_RESPONSE_CANCEL, "Open",
                        GTK_RESPONSE_ACCEPT, NULL); /* Create dialog. */

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE); /* Alow to select many files at once. */

    gint res = gtk_dialog_run (GTK_DIALOG (dialog) );
    if (res == GTK_RESPONSE_ACCEPT)
    {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);

        GSList *list = gtk_file_chooser_get_filenames (chooser);
        g_slist_foreach (list, (GFunc) raider_window_open, window);
        g_slist_free (list);
    }

    gtk_widget_destroy (dialog);
}


/*
 * THE BIG FUNCTION
 *
 * This is called when the user clicks on the Shred button in the options pane.
 */
void shred_file(GtkWidget *widget, gpointer data)
{
    /* Clear the subtitle. */
    RaiderWindow *window = RAIDER_WINDOW(data);
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window->header_bar), NULL);

    /* Launch the shredding. */
    GList *list = gtk_container_get_children(GTK_CONTAINER(window->list_box));
    g_list_foreach(list, launch, NULL);
    g_list_free(list);
}


RaiderWindow *
raider_window_new (Raider *app)
{
    return g_object_new (RAIDER_WINDOW_TYPE, "application", app, NULL);
}

/**********************************************************************************/


