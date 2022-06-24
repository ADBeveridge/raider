#include <gtk/gtk.h>

#include "raider.h"
#include "raider-window.h"
#include "raider-preferences.h"

struct _RaiderPreferences {
  AdwPreferencesWindow parent;

  GtkSwitch* remove_file_switch;
};

G_DEFINE_TYPE(RaiderPreferences, raider_preferences, ADW_TYPE_PREFERENCES_WINDOW)

static void raider_preferences_class_init(RaiderPreferencesClass *class)
{
  gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/com/github/ADBeveridge/Raider/raider-preferences.ui");
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), RaiderPreferences, remove_file_switch);
}


static void raider_preferences_init(RaiderPreferences *prefs)
{
  gtk_widget_init_template(GTK_WIDGET(prefs));
}


