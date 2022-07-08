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
	RaiderProgressIcon *icon;
	RaiderProgressInfoPopover *popover;

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
	guint signal_id;
	gboolean aborted;
};

G_DEFINE_TYPE(RaiderFileRow, raider_file_row, ADW_TYPE_ACTION_ROW)

gchar *raider_file_row_get_filename(RaiderFileRow * row)
{
	return g_file_get_path(row->file);
}

/* This is called when the user clicks on the remove button. */
void raider_file_row_delete(GtkWidget *widget, gpointer data)
{
	raider_window_close(data, GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(data))));

	GtkListBox *list_box = GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(data)));
	gtk_list_box_remove(list_box, GTK_WIDGET(data));
}

/* This is called when the shred executable exits, even if it is aborted. */
void finish_shredding(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	RaiderFileRow *row = RAIDER_FILE_ROW(user_data);

	if (!row->aborted) {
		GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(row)));
		if (!GTK_IS_WINDOW(toplevel))
			return;
		GApplication *app = G_APPLICATION(gtk_window_get_application(GTK_WINDOW(toplevel)));
		g_application_send_notification(app, NULL, row->notification);
	}

	raider_file_row_delete(NULL, user_data);
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
		g_error("Process launching failed: %s", error->message);
		g_error_free(error);
		return;
	}

	/* This parses the output. */
	GInputStream *stream = g_subprocess_get_stderr_pipe(row->process);
	row->backend = g_object_new(RAIDER_TYPE_SHRED_BACKEND, "data-stream", g_data_input_stream_new(stream), NULL);

	/* Change the button. */
	gtk_revealer_set_reveal_child(row->remove_revealer, FALSE);
	gtk_revealer_set_reveal_child(row->progress_revealer, TRUE);

	/* Call the callback when the process is finished. If the user aborts the
	   the job, this will be called in any event.*/
	g_subprocess_wait_async(row->process, NULL, (GAsyncReadyCallback)finish_shredding, row);
}

/** Widget related tasks. */
/* This is called when the user clicks abort. */
void raider_file_row_shredding_abort(gpointer data)
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

	/* Create the progress icon for the file row. */
	row->icon = g_object_new(RAIDER_TYPE_PROGRESS_ICON, NULL);
	row->popover = raider_progress_info_popover_new(GTK_WIDGET(row->progress_button));
	gtk_widget_set_parent(GTK_WIDGET(row->popover), GTK_WIDGET(row->progress_button));
	gtk_button_set_child(row->progress_button, GTK_WIDGET(row->icon));
	g_signal_connect(row->progress_button, "clicked", G_CALLBACK(raider_popup_popover), row->popover);

	/* Setup remove row. */
	g_signal_connect(row->remove_button, "clicked", G_CALLBACK(raider_file_row_delete), row);

	row->settings = g_settings_new("com.github.ADBeveridge.Raider");
	row->aborted = FALSE;
}

static void
raider_file_row_class_init(RaiderFileRowClass *klass)
{
	G_OBJECT_CLASS(klass)->dispose = raider_file_row_dispose; /* Override. */

	gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/Raider/raider-file-row.ui");
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, remove_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, remove_revealer);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_revealer);
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
