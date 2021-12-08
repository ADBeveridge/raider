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
}

RaiderPreferences *
raider_preferences_new (RaiderWindow *win)
{
    return g_object_new (RAIDER_PREFERENCES_TYPE, "transient-for", win, "use-header-bar", TRUE, NULL);
}
