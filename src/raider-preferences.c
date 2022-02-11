#include <gtk/gtk.h>

#include "raider.h"
#include "raider-window.h"
#include "raider-preferences.h"

struct _RaiderPreferences
{
    HdyPreferencesWindow parent;
	GtkWidget *preferences_page_one;
	GtkWidget *basic_group;
	GtkWidget *remove_file_bar;
	GtkWidget *remove_file_switch;
	GtkWidget *hide_shredding_bar;
	GtkWidget *hide_shredding_switch;
	GtkWidget *number_of_passes_bar;
	GtkWidget *number_of_passes_spin_button;
	GtkWidget *advanced_group;
	GtkWidget *remove_method_bar;
	GtkWidget *remove_method_combo;
	GtkWidget *overwrite_data_source_bar;
	GtkWidget *overwrite_data_source_file_chooser;
	GtkWidget *number_of_bytes_to_shred_bar;
	GtkWidget *number_of_bytes_to_shred_entry;
	GtkWidget *do_not_round_bar;
	GtkWidget *do_not_round_switch;
	GtkWidget *override_permissions_bar;
	GtkWidget *override_permissions_switch;

	GSettings *settings;
};

G_DEFINE_TYPE (RaiderPreferences, raider_preferences, HDY_TYPE_PREFERENCES_WINDOW)

static void
raider_preferences_init (RaiderPreferences *prefs)
{
    gtk_widget_init_template (GTK_WIDGET (prefs));

	prefs->settings = g_settings_new ("com.github.ADBeveridge.Raider");

	g_settings_bind (prefs->settings, "hide-shredding",
                     prefs->hide_shredding_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

	g_settings_bind (prefs->settings, "remove-file",
                     prefs->remove_file_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

	g_settings_bind (prefs->settings, "number-of-passes",
                     prefs->number_of_passes_spin_button, "value",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "remove-method",
                     prefs->remove_method_combo, "active-id",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "override-permissions",
                     prefs->override_permissions_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "do-not-round-to-next-block",
                     prefs->do_not_round_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "number-of-bytes-to-shred",
                     prefs->number_of_bytes_to_shred_entry, "text",
                     G_SETTINGS_BIND_DEFAULT);

	g_settings_bind (prefs->settings, "overwrite-data-file",
                     prefs->overwrite_data_source_file_chooser, "text",
                     G_SETTINGS_BIND_DEFAULT);
}

static void
raider_preferences_dispose (GObject *object)
{
    G_OBJECT_CLASS (raider_preferences_parent_class)->dispose (object);
}

static void
raider_preferences_class_init (RaiderPreferencesClass *class)
{
    G_OBJECT_CLASS (class)->dispose = raider_preferences_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class), "/com/github/ADBeveridge/raider/ui/raider-preferences.ui");
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, preferences_page_one);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, basic_group);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, remove_file_bar);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, remove_file_switch);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, hide_shredding_bar);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, hide_shredding_switch);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, number_of_passes_bar);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, number_of_passes_spin_button);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, advanced_group);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, remove_method_bar);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, remove_method_combo);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, overwrite_data_source_bar);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, overwrite_data_source_file_chooser);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, number_of_bytes_to_shred_bar);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, number_of_bytes_to_shred_entry);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, do_not_round_bar);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, do_not_round_switch);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, override_permissions_bar);
	gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, override_permissions_switch);
}

RaiderPreferences *
raider_preferences_new (RaiderWindow *win)
{
    return g_object_new (RAIDER_PREFERENCES_TYPE, "transient-for", win, NULL);
}
