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

#include "raider.h"
#include "raider-window.h"
#include "raider-preferences.h"

struct _RaiderPreferences {
	AdwPreferencesWindow parent;

	GtkSwitch* remove_file_switch;
    GtkSwitch* hide_shredding_switch;
    GtkSwitch* override_permissions_switch;
    GtkSwitch* dnr_switch;
    GtkSpinButton* nop_spin_button;
    GtkSpinButton* nob_spin_button;
    AdwComboRow* remove_method_combo_row;

    GSettings* settings;
};

G_DEFINE_TYPE(RaiderPreferences, raider_preferences, ADW_TYPE_PREFERENCES_WINDOW)

static void raider_preferences_class_init(RaiderPreferencesClass *class)
{
	gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/com/github/ADBeveridge/Raider/raider-preferences.ui");
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), RaiderPreferences, remove_file_switch);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), RaiderPreferences, hide_shredding_switch);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), RaiderPreferences, override_permissions_switch);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), RaiderPreferences, dnr_switch);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), RaiderPreferences, nop_spin_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), RaiderPreferences, nob_spin_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), RaiderPreferences, remove_method_combo_row);
}


static void raider_preferences_init(RaiderPreferences *prefs)
{
	gtk_widget_init_template(GTK_WIDGET(prefs));

    /* GSettings bindings. */
	prefs->settings = g_settings_new ("com.github.ADBeveridge.Raider");
	g_settings_bind (prefs->settings, "hide-shredding",
                     prefs->hide_shredding_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

	g_settings_bind (prefs->settings, "remove-file",
                     prefs->remove_file_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

	g_settings_bind (prefs->settings, "number-of-passes",
                     prefs->nop_spin_button, "value",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "remove-method",
                     prefs->remove_method_combo_row, "selected",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "override-permissions",
                     prefs->override_permissions_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "do-not-round-to-next-block",
                     prefs->dnr_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "number-of-bytes-to-shred",
                     prefs->nob_spin_button, "value",
                     G_SETTINGS_BIND_DEFAULT);

    /* Until I can bind the current file in it, this will not show up, and this is hidden in the .ui file. */
	/*g_settings_bind (prefs->settings, "overwrite-data-file",
                     prefs->overwrite_data_source_file_chooser, "text",
                     G_SETTINGS_BIND_DEFAULT);*/
}

