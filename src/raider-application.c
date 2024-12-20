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

struct _RaiderApplication
{
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

static void on_open_response(GObject* source_object, GAsyncResult* res, gpointer user_data)
{
    GListModel* list = gtk_file_dialog_open_multiple_finish (GTK_FILE_DIALOG(source_object), res, NULL);
    if (list == NULL) return;

    /* Convert g_list_model to g_list. */
    GList *file_list = NULL;
    int num = g_list_model_get_n_items(list);
    int i;
    for (i = 0; i < num; i++) {
        file_list = g_list_append(file_list, g_list_model_get_item(list, i));
    }
    g_object_unref(list);

    raider_window_open_files(RAIDER_WINDOW(user_data), file_list);
}

static void on_open_folder_response(GObject* source_object, GAsyncResult* res, gpointer user_data)
{
    GListModel* list = gtk_file_dialog_select_multiple_folders_finish (GTK_FILE_DIALOG(source_object), res, NULL);
    if (list == NULL) return;

    /* Convert g_list_model to g_list. */
    GList *file_list = NULL;
    int num = g_list_model_get_n_items(list);
    int i;
    for (i = 0; i < num; i++) {
        file_list = g_list_append(file_list, g_list_model_get_item(list, i));
    }
    g_object_unref(list);

    raider_window_open_files(RAIDER_WINDOW(user_data), file_list);
}

static void raider_application_open_to_window(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(user_data));

    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_modal(dialog, TRUE);
    gtk_file_dialog_set_title(dialog, _("Add Files"));
    gtk_file_dialog_open_multiple(dialog, GTK_WINDOW(window), NULL, on_open_response, window);
}

static void raider_application_open_folder_to_window(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(user_data));

    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_modal(dialog, TRUE);
    gtk_file_dialog_set_title(dialog, _("Add Folders"));
    gtk_file_dialog_select_multiple_folders(dialog, GTK_WINDOW(window), NULL, on_open_folder_response, window);
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

static void raider_application_close (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(user_data));
    gtk_window_close(window);
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

    adw_show_about_dialog (GTK_WIDGET(window),
                         "application-name", program_name,
                         "application-icon", "com.github.ADBeveridge.Raider",
                         "version", "3.0.2",
                         "copyright", "© 2024 Alan Beveridge",
                         "issue-url", "https://github.com/ADBeveridge/raider/issues/new",
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "developer-name", "Alan Beveridge",
                         "developers", developers,
                         "artists", artists,
                         "designers", designers,
                         "translator-credits", _("translator-credits"),
                         NULL);
}

static void raider_application_open(GApplication *application, GFile **files, gint n_files, const gchar *hint)
{
    RaiderWindow *window = g_object_new(RAIDER_TYPE_WINDOW, "application", application, NULL);

    /* Convert array of files into a GList. */
    GList *file_list = NULL;
    int i;
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
    gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.quit", (const char *[]){"<Ctrl>q",NULL,});

    g_autoptr(GSimpleAction) close_action = g_simple_action_new("close", NULL);
    g_signal_connect(close_action, "activate", G_CALLBACK(raider_application_close), self);
    g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(close_action));
    gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.close", (const char *[]){"<Ctrl>w",NULL,});

    g_autoptr(GSimpleAction) about_action = g_simple_action_new("about", NULL);
    g_signal_connect(about_action, "activate", G_CALLBACK(raider_application_show_about), self);
    g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(about_action));

    g_autoptr(GSimpleAction) new_window_action = g_simple_action_new("new-window", NULL);
    g_signal_connect(new_window_action, "activate", G_CALLBACK(raider_new_window), self);
    g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(new_window_action));
    gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.new-window", (const char *[]){"<Ctrl>n",NULL,});

    g_autoptr(GSimpleAction) open_action = g_simple_action_new("open", NULL);
    g_signal_connect(open_action, "activate", G_CALLBACK(raider_application_open_to_window), self);
    g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(open_action));
    gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.open", (const char *[]){"<Ctrl>o",NULL,});

    g_autoptr(GSimpleAction) open_folder_action = g_simple_action_new("open-folder", NULL);
    g_signal_connect(open_folder_action, "activate", G_CALLBACK(raider_application_open_folder_to_window), self);
    g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(open_folder_action));

    /* NOTE: NOT USED BECAUSE FLATPAK REMOVES ACCESS TO DEVICE FILES. */
    /*g_autoptr(GSimpleAction) open_drive_action = g_simple_action_new("open-drive", G_VARIANT_TYPE_STRING);
    g_signal_connect(open_drive_action, "activate", G_CALLBACK(raider_application_open_drive), self);
    g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(open_drive_action));*/
}

