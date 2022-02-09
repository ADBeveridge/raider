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
	GtkWidget *do_not_round_switchoverride_permissions_bar;
	GtkWidget *override_permissions_bar;
	GtkWidget *override_permissions_switch;
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
