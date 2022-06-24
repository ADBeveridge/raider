#include <gtk/gtk.h>
#include <adwaita.h>

#include "raider.h"
#include "raider-window.h"
#include "raider-preferences.h"

struct _RaiderPreferences {
	AdwPreferencesWindow parent;

	GtkSwitch* remove_file_switch;
    AdwComboRow* remove_method_row;
};

G_DEFINE_TYPE(RaiderPreferences, raider_preferences, ADW_TYPE_PREFERENCES_WINDOW)

static void raider_preferences_class_init(RaiderPreferencesClass *class)
{
	gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/com/github/ADBeveridge/Raider/raider-preferences.ui");
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), RaiderPreferences, remove_file_switch);
}


static void raider_preferences_init(RaiderPreferences *prefs)
{
	gtk_widget_init_template(GTK_WIDGET(prefs));

    GtkStringList *slist = gtk_string_list_new(NULL);
    gtk_string_list_append(slist, "Wipesync");
    gtk_string_list_append(slist, "Wipe");
    gtk_string_list_append(slist, "Unlink");


    remove_method_row = adw_combo_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(remove_method_row), "Remove Method");
    adw_combo_row_set_model(ADW_COMBO_ROW(combo), G_LIST_MODEL(slist));
}

