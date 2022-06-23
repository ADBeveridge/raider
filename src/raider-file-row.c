#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib/gi18n.h>
#include "raider-window.h"
#include "raider-file-row.h"
#include "raider-progress-icon.h"
#include "raider-shred-backend.h"

struct _RaiderFileRow {
	AdwActionRow parent;

	/* Graphical controls. */
	GtkButton* progress_button;
	GtkRevealer* progress_revealer;

	RaiderProgressIcon* icon;
	GtkButton* remove_button;
	GtkRevealer* remove_revealer;

	/* Notification widget. */
	GNotification *notification;
	gchar *notification_title;
	gchar *notification_subtitle;

	/* Data items. */
	GSettings *settings;
	gchar *filename;
	gchar *basename;
	GDataInputStream *data_stream;
	bool cont_parsing;
	GSubprocess *process;
	guint signal_id;
	gint timout_id;
	gboolean aborted;
};

G_DEFINE_TYPE(RaiderFileRow, raider_file_row, ADW_TYPE_ACTION_ROW)


/* This is called when the user clicks on the remove button. */
void raider_file_row_delete(GtkWidget* widget, gpointer data)
{
	GtkListBox* list_box = GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(data)));

	gtk_list_box_remove(list_box, GTK_WIDGET(data));
}


/** Shredding functions */
/* This function is called every few seconds to read the output of shred. */
void process_output_finish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	RaiderFileRow *row = RAIDER_FILE_ROW(user_data);

	gchar *buf = g_data_input_stream_read_line_finish(row->data_stream, res, NULL, NULL);

	/* If there is no data read in or available, return immediately. */
	if (buf == NULL) {
		return;
	}

	analyze_progress(buf, GTK_WIDGET(row->icon), NULL, row->filename, row->settings);
}

gboolean process_output(gpointer data)
{
	/* Converting the stream to text. */
	RaiderFileRow *row = RAIDER_FILE_ROW(data);

	if (row->cont_parsing) {
		g_data_input_stream_read_line_async(row->data_stream, G_PRIORITY_DEFAULT, NULL, process_output_finish, data);
	}

	return row->cont_parsing;
}

/* This is called when the shred executable exits, even if it is aborted. */
void finish_shredding(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	RaiderFileRow *file_row = RAIDER_FILE_ROW(user_data);

	/* Remove the timeout. */
	gboolean removed_timeout = g_source_remove(file_row->timout_id);
	if (removed_timeout == FALSE) {
		g_printerr("Could not stop timeout.\n");
	}
    file_row->cont_parsing = FALSE;

	if (!file_row->aborted) {
		GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(file_row)));
		if (!GTK_IS_WINDOW(toplevel)) return;
		GApplication *app = G_APPLICATION(gtk_window_get_application(GTK_WINDOW(toplevel)));
		g_application_send_notification(app, NULL, file_row->notification);
	}

	raider_file_row_delete(NULL, user_data);
}

void on_timeout_finished(gpointer user_data)
{
}

/* Invoked in raider-window.c. */
void launch_shredding(gpointer data)
{
	RaiderFileRow *file_row = RAIDER_FILE_ROW(data);
	GError *error = NULL;

	/* One liner settings. */
	gboolean remove_file = g_settings_get_boolean(file_row->settings, "remove-file");
	gboolean hide_shredding = g_settings_get_boolean(file_row->settings, "hide-shredding");
	gboolean override_permissions = g_settings_get_boolean(file_row->settings, "override-permissions");
	gboolean do_not_round_to_next_block = g_settings_get_boolean(file_row->settings, "do-not-round-to-next-block");
	gchar *shred_executable = g_settings_get_string(file_row->settings, "shred-executable");
	gboolean do_number_of_bytes_to_shred_command = TRUE;

	/* Get the number of passes. */
	gchar *number_of_passes = g_strdup_printf("%d", g_settings_get_int(file_row->settings, "number-of-passes"));
	gchar *number_of_passes_option = g_strconcat("--iterations=", number_of_passes, NULL);

	/* Get the remove method. The correspondings for each number is defined here. */
	gchar *remove_method;

	switch (g_settings_get_int(file_row->settings, "remove-method")) {
	case 0:
	{
		remove_method = g_strdup("wipesync");
		break;
	}
	case 1:
	{
		remove_method = g_strdup("wipe");
		break;
	}
	case 2:
	{
		remove_method = g_strdup("unlink");
		break;
	}
	}
	gchar *remove_method_command = g_strconcat("--remove=", remove_method, NULL);

	/* If that user requests, shred part of a file. */
	int number_of_bytes_to_shred = g_settings_get_int(file_row->settings, "number-of-bytes-to-shred");

	if (number_of_bytes_to_shred == 0) do_number_of_bytes_to_shred_command = FALSE;
	gchar *number_of_bytes_to_shred_converted = g_strdup_printf("%d", number_of_bytes_to_shred);
	gchar *number_of_bytes_to_shred_command = g_strconcat("--size=", number_of_bytes_to_shred_converted, NULL);

	/* Launch the shred executable on one file. There is a bit of a hack, as we substituted --verbose
	   for the commands that are absent in this launch. There is no error as shred does not complain
	   about too many --verbose'es */
	file_row->process = g_subprocess_new(G_SUBPROCESS_FLAGS_STDERR_PIPE, &error,
					     shred_executable, "--verbose", file_row->filename,
					     number_of_passes_option,
					     remove_file ? remove_method_command : "--verbose",
					     hide_shredding ? "--zero" : "--verbose",
					     override_permissions ? "--force" : "--verbose",
					     do_not_round_to_next_block ? "--exact" : "--verbose",
					     do_number_of_bytes_to_shred_command ? number_of_bytes_to_shred_command : "--verbose",
					     NULL);
	/* Free allocated text. */
	g_free(number_of_passes);
	g_free(number_of_passes_option);
	g_free(remove_method);
	g_free(remove_method_command);
	g_free(shred_executable);
	g_free(number_of_bytes_to_shred_converted);
	g_free(number_of_bytes_to_shred_command);

	/* Avoid dangling pointer references. */
	number_of_passes = NULL;
	number_of_passes_option = NULL;
	remove_method = NULL;
	remove_method_command = NULL;
	shred_executable = NULL;
	number_of_bytes_to_shred_converted = NULL;
	number_of_bytes_to_shred_command = NULL;

	if (error != NULL) {
		g_error("Process launching failed: %s", error->message);
		g_error_free(error);
		return;
	}

	/* This parses the output. */
	GInputStream *stream = g_subprocess_get_stderr_pipe(file_row->process);
	file_row->data_stream = g_data_input_stream_new(stream);

	/* Change the button. */
	gtk_revealer_set_reveal_child(file_row->remove_revealer, FALSE);
	gtk_revealer_set_reveal_child(file_row->progress_revealer, TRUE);

	/* Check the output every 100 milliseconds. Also create a cancellable. */
	file_row->cont_parsing = TRUE;
	file_row->timout_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 100, process_output, data, on_timeout_finished);

	/* Call the callback when the process is finished. If the user aborts the
	   the job, this will be called in any event.*/
	g_subprocess_wait_async(file_row->process, NULL, (GAsyncReadyCallback)finish_shredding, file_row);
}

/** Widget related tasks. */
/* This is called when the user clicks abort. */
void raider_file_row_shredding_abort(GtkWidget *widget, gpointer data)
{
	RaiderFileRow *row = RAIDER_FILE_ROW(data);

	row->aborted = TRUE;
	g_subprocess_force_exit(row->process);

	/* finish_shredding will be called here because when the subprocess
	   is done, finish_shredding is always called, regardless of the exit type. */
}

/* This is shown when the user clicks on the progress icon. The popover shows more information. */
void raider_popup_popover(GtkWidget *widget, gpointer data)
{
	gtk_popover_popup(GTK_POPOVER(data));
}

static void
raider_file_row_dispose(GObject *obj)
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

	G_OBJECT_CLASS(raider_file_row_parent_class)->dispose(obj);
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
	G_OBJECT_CLASS(klass)->dispose = raider_file_row_dispose;  /* Override. */

	gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/Raider/raider-file-row.ui");
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, remove_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, remove_revealer);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_revealer);
}

RaiderFileRow *raider_file_row_new(const char *str)
{
	RaiderFileRow *file_row = g_object_new(RAIDER_TYPE_FILE_ROW, NULL);

	file_row->filename = g_strdup(str);
	file_row->basename = g_path_get_basename(str);

	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(file_row), file_row->basename);
	adw_action_row_set_subtitle(ADW_ACTION_ROW(file_row), file_row->filename);

	/* Notification stuff. */
	file_row->notification_title = g_strconcat(_("Shredded "), file_row->basename, NULL);
	file_row->notification = g_notification_new(file_row->notification_title);
	file_row->notification_subtitle = NULL;

	return file_row;
}

