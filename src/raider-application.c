/* raider-application.c
 *
 * Copyright 2022 Alan Beveridge
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gio/gunixmounts.h>
#include <glib/gi18n.h>
#include "raider-application.h"
#include "raider-window.h"
#include "raider-preferences.h"

struct _RaiderApplication {
	AdwApplication parent_instance;
};

G_DEFINE_TYPE(RaiderApplication, raider_application, ADW_TYPE_APPLICATION)

RaiderApplication *raider_application_new(gchar * application_id, GApplicationFlags flags)
{
	return g_object_new(RAIDER_TYPE_APPLICATION, "application-id", application_id, "flags", flags, NULL);
}

/* Actions. */
static void raider_new_window(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	RaiderWindow *window = g_object_new(RAIDER_TYPE_WINDOW, "application", GTK_APPLICATION(user_data), NULL);

	gtk_window_present(GTK_WINDOW(window));
}

static void raider_application_preferences(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	RaiderPreferences *preferences = g_object_new(RAIDER_TYPE_PREFERENCES, "transient-for", gtk_application_get_active_window(GTK_APPLICATION(user_data)), NULL);
	gtk_window_present(GTK_WINDOW(preferences));
}

static void on_open_response(GtkDialog *dialog, int response)
{
	if (response == GTK_RESPONSE_ACCEPT) {
		GListModel *list = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));

		/* Convert g_list_model to g_list. */
		GList *file_list = NULL;
		int num = g_list_model_get_n_items(list);
		int i;
		for (i = 0; i < num; i++) {
			file_list = g_list_append(file_list, g_list_model_get_item(list, i));
		}
		g_object_unref(list);

		raider_window_open_files(RAIDER_WINDOW(gtk_window_get_transient_for(GTK_WINDOW(dialog))), file_list);
	}

	gtk_window_destroy(GTK_WINDOW(dialog));
}

static void raider_application_open_to_window(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(user_data));

	GtkDialog *dialog = GTK_DIALOG(gtk_file_chooser_dialog_new(_("Add Files"), GTK_WINDOW(window),
								   GTK_FILE_CHOOSER_ACTION_OPEN, _("Cancel"), GTK_RESPONSE_CANCEL, _("Add"),
								   GTK_RESPONSE_ACCEPT, NULL));

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

	g_signal_connect_swapped(dialog, "response", G_CALLBACK(on_open_response), dialog);
	gtk_widget_show(GTK_WIDGET(dialog));
}

static void raider_application_try_exit (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	GList* list = gtk_application_get_windows(GTK_APPLICATION(user_data));
	gboolean do_exit = TRUE;
	GList *l;
	for (l = list; l != NULL; l = l->next)
  	{
    	if (raider_window_exit(RAIDER_WINDOW(l->data), NULL) == TRUE)
		{
			/* If this executes, shredding is ongoing. */
            do_exit = FALSE;
		}
  	}
	g_list_free(list);

	if (do_exit)
	{
		g_application_quit(G_APPLICATION(user_data));
	}
}

/* NOTE: NOT USED BECAUSE FLATPAK REMOVES ACCESS TO DEVICE FILES. */
/*static void raider_application_open_drive(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(user_data));

	GUnixMountEntry *unix_mount = g_unix_mount_at(g_variant_get_string(parameter, NULL), NULL);
	const gchar *path = g_unix_mount_get_device_path(unix_mount); // This returns something like /dev/sdb1.

	GFile *title = g_file_new_for_path(g_variant_get_string(parameter, NULL));
	gchar* name = g_file_get_basename(title);
	g_object_unref(title);

	raider_window_open(g_file_new_for_path(path), window, name);

	g_free(name);
}
*/

static void raider_application_show_about(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	RaiderApplication *self = RAIDER_APPLICATION(user_data);
	GtkWindow *window = NULL;

	g_return_if_fail(RAIDER_IS_APPLICATION(self));
	window = gtk_application_get_active_window(GTK_APPLICATION(self));

	const gchar *artists[] =
	{
		"noëlle https://github.com/jannuary",
		"David Lapshin https://github.com/daudix-UFO",
		NULL
	};
	const gchar *developers[] =
	{
		"Alan Beveridge https://github.com/ADBeveridge",
		NULL
	};
    const gchar *designers[] =
	{
		"Tobias Bernard https://gitlab.gnome.org/bertob",
		NULL
	};

	g_autofree gchar *program_name = g_strdup(_("File Shredder"));

    adw_show_about_window (window,
                         "transient-for", window,
                         "application-name", program_name,
                         "application-icon", "com.github.ADBeveridge.Raider",
                         "version", "1.3.0",
                         "copyright", "© 2022 Alan Beveridge",
                         "issue-url", "https://github.com/ADBeveridge/raider/issues/",
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "developer-name", "Alan Beveridge",
                         "developers", developers,
                         "artists", artists,
                         "designers", designers,
                         "translator-credits", _("translator-credits"),
                         NULL);
}

static void raider_application_show_help(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	gtk_show_uri(gtk_application_get_active_window(GTK_APPLICATION(user_data)), "help:raider", GDK_CURRENT_TIME);
}

static void raider_application_open(GApplication *application, GFile **files, gint n_files, const gchar *hint)
{
	RaiderWindow *window = g_object_new(RAIDER_TYPE_WINDOW, "application", application, NULL);
	gint i;

	/* Convert array of files into a GList. */
	GList *file_list = NULL;
	for (i = 0; i < n_files; i++) {
		file_list = g_list_append(file_list, g_file_dup(files[i]));
	}

	raider_window_open_files(window, file_list);

	gtk_window_present(GTK_WINDOW(window));
}

static void raider_application_finalize(GObject *object)
{
	G_OBJECT_CLASS(raider_application_parent_class)->finalize(object);
}

static void raider_application_activate(GApplication *app)
{
	g_assert(GTK_IS_APPLICATION(app));

	/* Get the current window or create one if necessary. */
	GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(app));
	if (window == NULL)
		window = g_object_new(RAIDER_TYPE_WINDOW, "application", app, NULL);

	gtk_window_present(window);
}

static void raider_application_class_init(RaiderApplicationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GApplicationClass *app_class = G_APPLICATION_CLASS(klass);

	object_class->finalize = raider_application_finalize;
	app_class->activate = raider_application_activate;
	app_class->open = raider_application_open;
}

static void raider_application_init(RaiderApplication *self)
{
	g_autoptr(GSimpleAction) quit_action = g_simple_action_new("quit", NULL);
	g_signal_connect(quit_action, "activate", G_CALLBACK(raider_application_try_exit), self);
	g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(quit_action));

	g_autoptr(GSimpleAction) about_action = g_simple_action_new("about", NULL);
	g_signal_connect(about_action, "activate", G_CALLBACK(raider_application_show_about), self);
	g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(about_action));

	g_autoptr(GSimpleAction) new_window_action = g_simple_action_new("new_window", NULL);
	g_signal_connect(new_window_action, "activate", G_CALLBACK(raider_new_window), self);
	g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(new_window_action));

	g_autoptr(GSimpleAction) open_action = g_simple_action_new("open", NULL);
	g_signal_connect(open_action, "activate", G_CALLBACK(raider_application_open_to_window), self);
	g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(open_action));

	g_autoptr(GSimpleAction) preferences_action = g_simple_action_new("preferences", NULL);
	g_signal_connect(preferences_action, "activate", G_CALLBACK(raider_application_preferences), self);
	g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(preferences_action));

	g_autoptr(GSimpleAction) help_action = g_simple_action_new("help", NULL);
	g_signal_connect (help_action, "activate", G_CALLBACK(raider_application_show_help), self);
	g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(help_action));

    /* NOTE: NOT USED BECAUSE FLATPAK REMOVES ACCESS TO DEVICE FILES. */
    /*g_autoptr(GSimpleAction) open_drive_action = g_simple_action_new("open-drive", G_VARIANT_TYPE_STRING);
	g_signal_connect(open_drive_action, "activate", G_CALLBACK(raider_application_open_drive), self);
	g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(open_drive_action));*/

	gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.quit", (const char *[]){"<Ctrl>q",NULL,});
	gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.open", (const char *[]){"<Ctrl>o",NULL,});
	gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.help", (const char *[]){"F1",NULL,});
	gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.new_window", (const char *[]){"<Ctrl>n",NULL,});
	gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.preferences", (const char *[]){"<Ctrl>comma",NULL,});

	/* Always disable data-file, as user may forget that he loaded it. */
	GSettings* settings = g_settings_new("com.github.ADBeveridge.Raider");
	g_settings_set_boolean(settings, "do-data-file", FALSE);
	g_object_unref(settings);
}

