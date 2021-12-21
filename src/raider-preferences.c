#include <gtk/gtk.h>

#include "raider.h"
#include "raider-window.h"
#include "raider-preferences.h"

struct _RaiderPreferences
{
    GtkDialog parent;

    GSettings *settings;
    GtkWidget *hide_shredding_check_button;
    GtkWidget *remove_file_check_button;
    GtkWidget *number_of_passes_spin_button;
    GtkWidget *remove_method_combo_box;
    GtkWidget *overwrite_data_file_chooser_button;
    GtkWidget *override_permissions_check_button;
    GtkWidget *do_not_round_to_next_block_check_button;
    GtkWidget *number_of_bytes_to_shred_entry;
};

G_DEFINE_TYPE (RaiderPreferences, raider_preferences, GTK_TYPE_DIALOG)

static void
raider_preferences_init (RaiderPreferences *prefs)
{
    gtk_widget_init_template (GTK_WIDGET (prefs));
    prefs->settings = g_settings_new ("org.gnome.Raider");

    g_settings_bind (prefs->settings, "hide-shredding",
                     prefs->hide_shredding_check_button, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "remove-file",
                     prefs->remove_file_check_button, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "number-of-passes",
                     prefs->number_of_passes_spin_button, "value",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "remove-method",
                     prefs->remove_method_combo_box, "active-id",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "override-permissions",
                     prefs->override_permissions_check_button, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "do-not-round-to-next-block",
                     prefs->do_not_round_to_next_block_check_button, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (prefs->settings, "number-of-bytes-to-shred",
                     prefs->number_of_bytes_to_shred_entry, "text",
                     G_SETTINGS_BIND_DEFAULT);
}

static void
raider_preferences_dispose (GObject *object)
{
    RaiderPreferences *prefs = RAIDER_PREFERENCES (object);
    g_clear_object (&prefs->settings);

    G_OBJECT_CLASS (raider_preferences_parent_class)->dispose (object);
}

static void
raider_preferences_class_init (RaiderPreferencesClass *class)
{
    G_OBJECT_CLASS (class)->dispose = raider_preferences_dispose;

    gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class), "/org/gnome/raider/ui/raider-preferences.ui");
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, hide_shredding_check_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, remove_file_check_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, number_of_passes_spin_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, remove_method_combo_box);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, overwrite_data_file_chooser_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, override_permissions_check_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, do_not_round_to_next_block_check_button);
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, number_of_bytes_to_shred_entry);
}

RaiderPreferences *
raider_preferences_new (RaiderWindow *win)
{
    return g_object_new (RAIDER_PREFERENCES_TYPE, "transient-for", win, "use-header-bar", TRUE, NULL);
}
