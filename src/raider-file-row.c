#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib/gi18n.h>
#include "raider-file-row.h"

struct _RaiderFileRow
{
    AdwActionRow parent;
};

G_DEFINE_TYPE (RaiderFileRow, raider_file_row, ADW_TYPE_ACTION_ROW)


void raider_file_row_shredding_abort (GtkWidget *widget, gpointer data)
{
}

void raider_file_row_delete (GtkWidget *widget, gpointer data)
{
}

void raider_popup_popover (GtkWidget *widget, gpointer data)
{
}

static void
raider_file_row_init (RaiderFileRow *row)
{
}

static void
raider_file_row_dispose (GObject *obj)
{
}

static void
raider_file_row_class_init (RaiderFileRowClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = raider_file_row_dispose;

    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/raider/ui/raider-file-row.ui");
}

/** Utility functions **/
void finish_shredding (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
}

void launch (gpointer data, gpointer user_data)
{
}

