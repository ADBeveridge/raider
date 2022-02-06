#include <gtk/gtk.h>

#include "raider.h"
#include "raider-window.h"
#include "raider-preferences.h"

struct _RaiderPreferences
{
    HdyPreferencesWindow parent;
};

G_DEFINE_TYPE (RaiderPreferences, raider_preferences, HDY_TYPE_PREFERENCES_WINDOW)

static void
raider_preferences_init (RaiderPreferences *prefs)
{
    gtk_widget_init_template (GTK_WIDGET (prefs));
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
}

RaiderPreferences *
raider_preferences_new (RaiderWindow *win)
{
    return g_object_new (RAIDER_PREFERENCES_TYPE, "transient-for", NULL);
}
