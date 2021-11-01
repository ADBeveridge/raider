#include "raider-window.h"

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

    /* Custom variables initialization. */
    win->loaded_file_count = 0;
    win->how_many_done = 0;
    win->array_of_files = g_ptr_array_new();
    win->array_of_progress_bars = g_ptr_array_new();

    /* Make the treeview a DND destination. */
    static GtkTargetEntry targetentries[] = {{ "text/uri-list", 0, 0 }};
    gtk_drag_dest_set (win->tree_view, GTK_DEST_DEFAULT_ALL, targetentries, 1, GDK_ACTION_COPY); /* Make it into a dnd destination. */

    /* Settings. */
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
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, contents_box);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, shred_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, tree_view);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, list_store);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, hide_shredding_check_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, number_of_passes_spin_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, remove_file_check_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, progress_overlay_revealer);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, tree_view_overlay_revealer);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderWindow, progress_overlay_box);

    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), shred_file);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), on_drag_data_received);
    gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), open_file_dialog);
}

void
raider_window_open (RaiderWindow *win, GFile **files)
{
    // Add files to a NEW window.
}

RaiderWindow *
raider_window_new (Raider *app)
{
    return g_object_new (RAIDER_WINDOW_TYPE, "application", app, NULL);
}

/**********************************************************************************/


