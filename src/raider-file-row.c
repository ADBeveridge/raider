#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib/gi18n.h>
#include "raider-window.h"
#include "raider-file-row.h"
#include "raider-progress-icon.h"


struct _RaiderFileRow {
	AdwActionRow parent;

	/* Graphical controls. */
	GtkButton* progress_button;
	RaiderProgressIcon* icon;
	GtkButton* remove_button;

	/* Notification widget. */
    GNotification *notification;
    gchar *notification_title;
    gchar *notification_subtitle;

    /* Data items. */
    GSettings *settings;
    gchar *filename;
    gchar *basename;
    GDataInputStream *data_stream;
    GSubprocess *process;
    guint signal_id;
    gint timout_id;
    gboolean aborted;
};

G_DEFINE_TYPE(RaiderFileRow, raider_file_row, ADW_TYPE_ACTION_ROW)

/* This is called when the user clicks on the remove button. */
void raider_file_row_delete(GtkWidget* widget, gpointer data)
{
	GtkListBox* list_box = gtk_widget_get_parent(GTK_WIDGET(data));
	gtk_list_box_remove(list_box, GTK_WIDGET(data));
}

/* This is called when the user clicks abort. */
void raider_file_row_shredding_abort (GtkWidget *widget, gpointer data)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(data);
    row->aborted = TRUE;
    g_subprocess_force_exit(row->process);

    /* finish_shredding will be called here because when the subprocess
    is done, finish_shredding is always called, regardless of the exit type. */
}

/* This is shown when the user clicks on the progress icon. The popover shows more information. */
void raider_popup_popover (GtkWidget *widget, gpointer data)
{
    gtk_popover_popup(GTK_POPOVER(data));
}

static void
raider_file_row_dispose (GObject *obj)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(obj);
    g_free(row->filename);
    row->filename = NULL;
    g_free(row->basename);
    row->basename = NULL;
    g_free(row->notification_title);
    row->notification_title = NULL;
    g_free(row->notification_subtitle);
    row->notification_subtitle = NULL;

    G_OBJECT_CLASS (raider_file_row_parent_class)->dispose (obj);
}

static void
raider_file_row_init(RaiderFileRow *row)
{
	gtk_widget_init_template(GTK_WIDGET(row));

	/* Create the progress icon for the file row. */
	row->icon = g_object_new(RAIDER_TYPE_PROGRESS_ICON, NULL);
	raider_progress_icon_set_progress(row->icon, 0.5);
	gtk_button_set_child(row->progress_button, GTK_WIDGET(row->icon));

	g_signal_connect(row->remove_button, "clicked", G_CALLBACK(raider_file_row_delete), row);

	row->settings = g_settings_new("com.github.ADBeveridge.Raider");
    row->aborted = FALSE;
}

static void
raider_file_row_class_init(RaiderFileRowClass *klass)
{
	G_OBJECT_CLASS (klass)->dispose = raider_file_row_dispose; /* Override. */

	gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/Raider/raider-file-row.ui");
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, remove_button);
}

RaiderFileRow *raider_file_row_new (const char *str)
{
    RaiderFileRow *file_row = g_object_new (RAIDER_TYPE_FILE_ROW, NULL);

    file_row->filename = g_strdup(str);
    file_row->basename = g_path_get_basename(str);

	adw_preferences_row_set_title (ADW_PREFERENCES_ROW(file_row), file_row->basename);
	adw_action_row_set_subtitle(ADW_ACTION_ROW(file_row), file_row->filename);

    /* Notification stuff. */
    file_row->notification_title = g_strconcat(_("Shredded "), file_row->basename, NULL);
    file_row->notification = g_notification_new(file_row->notification_title);
    file_row->notification_subtitle = NULL;

    return file_row;
}


