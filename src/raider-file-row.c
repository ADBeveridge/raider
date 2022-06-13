#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib/gi18n.h>
#include "raider-file-row.h"
#include "raider-progress-icon.h"

struct _RaiderFileRow
{
    AdwActionRow parent;
};

G_DEFINE_TYPE (RaiderFileRow, raider_file_row, ADW_TYPE_ACTION_ROW)


static void
raider_file_row_init (RaiderFileRow *row)
{
  gtk_widget_init_template(GTK_WIDGET(row));
}

static void
raider_file_row_class_init (RaiderFileRowClass *klass)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/Raider/raider-file-row.ui");
}

