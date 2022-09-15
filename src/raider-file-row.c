/* raider-file-row.c
 *
 * Copyright 2022 Alan Beveridge
 *
 * raider is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * raider is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib/gi18n.h>
#include "raider-window.h"
#include "raider-file-row.h"
#include "raider-progress-icon.h"
#include "raider-shred-backend.h"
#include "raider-progress-info-popover.h"
#include "raider-shred-backend.h"

struct _RaiderFileRow {
	AdwActionRow parent;

	/* Graphical controls. */
	GtkButton *progress_button;
	GtkButton *remove_button;

	GtkRevealer *progress_revealer;
	GtkRevealer *remove_revealer;

	/* Progress widgets. */
	GtkBox* progress_widgets_box;
	RaiderProgressInfoPopover *popover;

	RaiderProgressIcon *icon;
	GtkWidget* spinner;

	/* Notification widget. */
	GNotification *notification;
	gchar *notification_title;
	gchar *notification_subtitle;

	/* Data items. */
	GSettings *settings;
	GFile *file;
	RaiderShredBackend* backend;
	bool cont_parsing;
	GSubprocess *process;
	guint timeout_id;
	guint signal_id;
	gboolean aborted;
};

G_DEFINE_TYPE(RaiderFileRow, raider_file_row, ADW_TYPE_ACTION_ROW)

gchar *raider_file_row_get_filename(RaiderFileRow * row)
{
	return g_file_get_path(row->file);
}
/* This version is called when the user has aborted the operation, like clicking the abort button or close button. */
void raider_file_row_delete_on_abort(GtkWidget *widget, gpointer data)
{
	raider_window_close_file(data, GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(data))), 0);
	GtkListBox *list_box = GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(data)));
	gtk_list_box_remove(list_box, GTK_WIDGET(data));
}
/* This version of delete tells the raider_window_close_file function to show a toast that shredding is done. */
void raider_file_row_delete_on_finish(GtkWidget *widget, gpointer data)
{
	raider_window_close_file(data, GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(data))), 1);
	GtkListBox *list_box = GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(data)));
	gtk_list_box_remove(list_box, GTK_WIDGET(data));
}

gboolean raider_file_row_update_progress(gpointer data)
{
	RaiderFileRow* row = RAIDER_FILE_ROW(data);
	gdouble progress = raider_shred_backend_get_progress(row->backend);

	if (progress == 0.0) {
		gtk_widget_set_visible(row->spinner, TRUE);
		gtk_widget_set_visible(GTK_WIDGET(row->icon), FALSE);
		raider_progress_info_popover_pulse(row->popover);
	}else {
		gtk_widget_set_visible(row->spinner, FALSE);
		gtk_widget_set_visible(GTK_WIDGET(row->icon), TRUE);
		raider_progress_icon_set_progress(row->icon, progress);
		raider_progress_info_popover_set_progress(row->popover, progress);
	}

	return TRUE;
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

	g_object_unref(row->file);
	g_object_unref(row->settings);

	gtk_widget_unparent(GTK_WIDGET(row->popover));

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

	row->popover = raider_progress_info_popover_new();
	gtk_widget_set_parent(GTK_WIDGET(row->popover), GTK_WIDGET(row->progress_button));
	g_signal_connect(row->progress_button, "clicked", G_CALLBACK(raider_popup_popover), row->popover);

	/* Setup remove row. */
	g_signal_connect(row->remove_button, "clicked", G_CALLBACK(raider_file_row_delete_on_abort), row);

	row->settings = g_settings_new("com.github.ADBeveridge.Raider");
	row->aborted = FALSE;
	row->process = NULL;
}

static void
raider_file_row_class_init(RaiderFileRowClass *klass)
{
	G_OBJECT_CLASS(klass)->dispose = raider_file_row_dispose; /* Override. */

	/* Load the progress icon into the type system. */
	g_type_ensure(RAIDER_TYPE_PROGRESS_ICON);

	gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/Raider/raider-file-row.ui");
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, remove_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, remove_revealer);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_revealer);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_widgets_box);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, spinner);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, icon);
}

RaiderFileRow *raider_file_row_new(GFile *file)
{
	RaiderFileRow *row = g_object_new(RAIDER_TYPE_FILE_ROW, NULL);

	row->file = file;

	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), g_file_get_basename(row->file));
	adw_action_row_set_subtitle(ADW_ACTION_ROW(row), g_file_get_path(row->file));

	/* Notification stuff. */
	row->notification_title = g_strconcat(_("Shredded "), g_file_get_basename(row->file), NULL);
	row->notification = g_notification_new(row->notification_title);
	row->notification_subtitle = NULL;

	return row;
}

void on_complete_finish(GObject* source_object, GAsyncResult* res, gpointer user_data)
{
	RaiderFileRow* row = RAIDER_FILE_ROW(user_data);
	gchar* message = raider_shred_backend_get_return_result_string(row->backend);

	/* If the message does not contain 'good' */
	if (g_strcmp0(message, "good") != 0) {
		row->aborted = TRUE;
		adw_action_row_set_subtitle(ADW_ACTION_ROW(row), message);
	}

	/* Make sure that the user can use the window after the row is destroyed, and delete the backend. */
	gtk_widget_hide(GTK_WIDGET(row->popover));
	g_object_unref(row->backend);
	g_object_unref(row->process);

	/* Remove the timeout. */
	gboolean removed_timeout = g_source_remove(row->timeout_id);
	if (removed_timeout == FALSE)
		g_warning("Could not stop timeout.\n");

	if (row->aborted == FALSE)
		raider_file_row_delete_on_finish(NULL, user_data);
	else { // If is was aborted.
		/* Reset the view of the file row. */
		adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), NULL);
		gtk_revealer_set_reveal_child(row->remove_revealer, TRUE);
		gtk_revealer_set_reveal_child(row->progress_revealer, FALSE);
		gtk_spinner_stop(GTK_SPINNER(row->spinner));
		row->aborted = FALSE; // For next time.
	}
}

/* This is called when the shred executable exits, even if it is aborted. */
static void finish_shredding(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	RaiderFileRow *row = RAIDER_FILE_ROW(user_data);
	raider_shred_backend_get_return_result((gpointer)row, on_complete_finish, row->backend);
}

/* Invoked in raider-window.c. nob stands for number of bytes. */
void raider_file_row_launch_shredding(gpointer data)
{
	RaiderFileRow *row = RAIDER_FILE_ROW(data);
	GError *error = NULL;

	/* One liner settings. */
	gboolean remove_file = g_settings_get_boolean(row->settings, "remove-file");
	gboolean hide_shredding = g_settings_get_boolean(row->settings, "hide-shredding");
	gboolean override_permissions = g_settings_get_boolean(row->settings, "override-permissions");
	gboolean do_not_round_to_next_block = g_settings_get_boolean(row->settings, "do-not-round-to-next-block");
	gchar *shred_executable = g_settings_get_string(row->settings, "shred-executable");
	gboolean do_nob_command = TRUE;

	/* Get the number of passes. */
	gchar *number_of_passes = g_strdup_printf("%d", g_settings_get_int(row->settings, "number-of-passes"));
	gchar *number_of_passes_option = g_strconcat("--iterations=", number_of_passes, NULL);
	g_free(number_of_passes);

	/* Get the remove method. The correspondings for each number is defined here. */
	gchar *remove_method;

	switch (g_settings_get_int(row->settings, "remove-method")) {
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
	g_free(remove_method);

	/* If that user requests, shred part of a file. */
	int nob = g_settings_get_int(row->settings, "number-of-bytes-to-shred");

	if (nob == 0)
		do_nob_command = FALSE;
	gchar *nob_tmp = g_strdup_printf("%d", nob);
	gchar *nob_command = g_strconcat("--size=", nob_tmp, NULL);
	g_free(nob_tmp);

	/* Get whether to do data file. */
	gboolean do_data_file = g_settings_get_boolean(row->settings, "do-data-file");
	gchar* data_file = g_settings_get_string(row->settings, "data-file");
	gchar* data_file_command = g_strconcat("--random-source=", data_file, NULL);
	g_free(data_file);

	/* Launch the shred executable on one file. There is a bit of a hack, as we substituted --verbose
	   for the commands that are absent in this launch. There is no error as shred does not complain
	   about too many --verbose'es */
	row->process = g_subprocess_new(G_SUBPROCESS_FLAGS_STDERR_PIPE, &error,
					shred_executable, "--verbose", g_file_get_path(row->file),
					number_of_passes_option,
					remove_file ? remove_method_command : "--verbose",
					hide_shredding ? "--zero" : "--verbose",
					override_permissions ? "--force" : "--verbose",
					do_not_round_to_next_block ? "--exact" : "--verbose",
					do_nob_command ? nob_command : "--verbose",
					do_data_file ? data_file_command : "--verbose",
					NULL);

	/* Free allocated text. */
	g_free(number_of_passes_option);
	g_free(remove_method_command);
	g_free(shred_executable);
	g_free(nob_command);

	if (error != NULL) {
		g_critical("Process launching failed: %s", error->message);
		g_error_free(error);
		g_object_unref(row->process);
		adw_action_row_set_subtitle(ADW_ACTION_ROW(row), _("Failed to start shredding"));
		return;
	}

	/* This parses the output. */
	GInputStream *stream = g_subprocess_get_stderr_pipe(row->process);
	row->backend = g_object_new(RAIDER_TYPE_SHRED_BACKEND,
				    "data-stream", g_data_input_stream_new(stream),
				    "settings", row->settings,
				    "filename", g_file_get_path(row->file), NULL);

	/* Change the button. */
	gtk_revealer_set_reveal_child(row->remove_revealer, FALSE);
	gtk_revealer_set_reveal_child(row->progress_revealer, TRUE);

	/* Start the spinner. */
	gtk_spinner_start(GTK_SPINNER(row->spinner));

	/* Set the activatable widget. */
	adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), GTK_WIDGET(row->progress_button));

	row->timeout_id = g_timeout_add(50, (GSourceFunc)raider_file_row_update_progress, data);

	/* Call the callback when the process is finished. If the user aborts the
	   the job, this will be called in any event.*/
	g_subprocess_wait_async(row->process, NULL, (GAsyncReadyCallback)finish_shredding, row);
}

/* This is called when the user clicks abort. */
void raider_file_row_shredding_abort(gpointer data)
{
	RaiderFileRow *row = RAIDER_FILE_ROW(data);

	row->aborted = TRUE;
	g_subprocess_force_exit(row->process);
	g_subprocess_wait(row->process, NULL, NULL);

	/* finish_shredding will be called here because when the subprocess
	   is done, finish_shredding is always called, regardless of the exit type. */
}

