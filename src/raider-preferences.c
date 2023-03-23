/* raider-preferences.c
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
#include "raider-application.h"
#include "raider-window.h"
#include "raider-preferences.h"

struct _RaiderPreferences
{
    AdwPreferencesWindow parent;

    GtkSwitch *remove_file_switch;

    GSettings *settings;
};

G_DEFINE_TYPE(RaiderPreferences, raider_preferences, ADW_TYPE_PREFERENCES_WINDOW)

static void raider_preferences_finalize(GObject *object)
{
    RaiderPreferences *prefs = RAIDER_PREFERENCES(object);
    g_object_unref(prefs->settings);

    G_OBJECT_CLASS(raider_preferences_parent_class)->finalize(object);
}

static void raider_preferences_class_init(RaiderPreferencesClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);
    object_class->finalize = raider_preferences_finalize;

    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/com/github/ADBeveridge/Raider/raider-preferences.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), RaiderPreferences, remove_file_switch);
}

static void raider_preferences_init(RaiderPreferences *prefs)
{
    gtk_widget_init_template(GTK_WIDGET(prefs));

    /* GSettings bindings. */
    prefs->settings = g_settings_new("com.github.ADBeveridge.Raider");
    g_settings_bind(prefs->settings, "remove-file",
                    prefs->remove_file_switch, "active",
                    G_SETTINGS_BIND_DEFAULT);

}

