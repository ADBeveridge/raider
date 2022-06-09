/* raider-application.c
 *
 * Copyright 2022 Alan
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

#include "raider-application.h"
#include "raider-window.h"
#include <glib/gi18n.h>

struct _RaiderApplication {
  GtkApplication parent_instance;
};

G_DEFINE_TYPE(RaiderApplication, raider_application, ADW_TYPE_APPLICATION)

RaiderApplication* raider_application_new(gchar * application_id,
                                          GApplicationFlags flags)
{
  return g_object_new(RAIDER_TYPE_APPLICATION, "application-id", application_id, "flags", flags, NULL);
}

/* Create a new window. */
static void raider_new_window(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  RaiderApplication *self = RAIDER_APPLICATION(user_data);

  RaiderWindow *window = g_object_new(RAIDER_TYPE_WINDOW, "application", self, NULL);

  gtk_window_present(GTK_WINDOW(window));
}

static void raider_application_finalize(GObject *object)
{
  RaiderApplication *self = (RaiderApplication *)object;

  G_OBJECT_CLASS(raider_application_parent_class)->finalize(object);
}

static void raider_application_activate(GApplication *app)
{
  GtkWindow *window;

  g_assert(GTK_IS_APPLICATION(app));

  /* Get the current window or create one if necessary. */
  window = gtk_application_get_active_window(GTK_APPLICATION(app));
  if (window == NULL)
    window = g_object_new(RAIDER_TYPE_WINDOW, "application", app, NULL);

  /* Ask the window manager/compositor to present the window. */
  gtk_window_present(window);
}

static void raider_application_open_to_window(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  GtkWindow* window = gtk_application_get_active_window(GTK_APPLICATION(user_data));
}

static void raider_application_class_init(RaiderApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS(klass);

  object_class->finalize = raider_application_finalize;
  app_class->activate = raider_application_activate;
}

static void raider_application_show_about(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  RaiderApplication *self = RAIDER_APPLICATION(user_data);
  GtkWindow *window = NULL;
  g_return_if_fail(RAIDER_IS_APPLICATION(self));
  window = gtk_application_get_active_window(GTK_APPLICATION(self));

  const gchar *artists[] =
  {
    "noëlle",
    NULL
  };
  const gchar *authors[] =
  {
    "Alan Beveridge",
    NULL
  };

  g_autofree gchar *program_name = g_strdup("File Shredder");
  gtk_show_about_dialog(window,
                        "transient-for", window,
                        "modal", TRUE,
                        "program-name", program_name,
                        "version", _("0.1.0"),
                        "comments", _("Securely delete your files"),
                        "license-type", GTK_LICENSE_GPL_3_0,
                        "artists", artists,
                        "authors", authors,
                        "translator-credits", _("translator-credits"),
                        "logo-icon-name", "com.github.ADBeveridge.Raider",
                        NULL);
}


static void raider_application_init(RaiderApplication *self)
{
  g_autoptr(GSimpleAction) quit_action = g_simple_action_new("quit", NULL);
  g_signal_connect_swapped(quit_action, "activate", G_CALLBACK(g_application_quit), self);
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

  gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.quit", (const char *[]) { "<primary>q", NULL, });
}
