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

struct _RaiderPreferences {
	AdwPreferencesWindow parent;

	GtkSwitch* remove_file_switch;
    GtkSwitch* hide_shredding_switch;
    GtkSwitch* override_permissions_switch;
    GtkSwitch* dnr_switch;
    GtkSpinButton* nop_spin_button;
    GtkSpinButton* nob_spin_button;
    AdwComboRow* remove_method_combo_row;
    GtkButton* sd_button;
    AdwActionRow* sd_row;

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
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), RaiderPreferences, sd_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), RaiderPreferences, sd_row);
}

static void on_open_response(GtkDialog *dialog, int response, gpointer user_data)
{
	RaiderPreferences* prefs = RAIDER_PREFERENCES(user_data);

    if (response == GTK_RESPONSE_ACCEPT) {
		GFile* file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER(dialog));
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(prefs->sd_row), g_file_get_path(file));
	}

	gtk_window_destroy(GTK_WINDOW(dialog));
}

static void raider_preferences_open (GtkWidget* widget, gpointer user_data)
{
	RaiderPreferences* prefs = RAIDER_PREFERENCES(user_data);

    GtkDialog *dialog = GTK_DIALOG(gtk_file_chooser_dialog_new(_("Open File"), GTK_WINDOW(prefs),
								   GTK_FILE_CHOOSER_ACTION_OPEN, _("Cancel"), GTK_RESPONSE_CANCEL, _("Open"),
								   GTK_RESPONSE_ACCEPT, NULL)); /* Create dialog. */

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

	g_signal_connect(dialog, "response", G_CALLBACK(on_open_response), user_data);
	gtk_widget_show(GTK_WIDGET(dialog));
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
    g_settings_bind (prefs->settings, "overwrite-data-file",
                     prefs->sd_row, "title",
                     G_SETTINGS_BIND_DEFAULT);

    /* Until I can bind the current file in it, this will not show up, and this is hidden in the .ui file. */
	/*g_settings_bind (prefs->settings, "overwrite-data-file",
                     prefs->overwrite_data_source_file_chooser, "text",
                     G_SETTINGS_BIND_DEFAULT);*/

    g_signal_connect(prefs->sd_button, "clicked", G_CALLBACK(raider_preferences_open), prefs);
}

