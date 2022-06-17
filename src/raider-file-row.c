#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib/gi18n.h>
#include "raider-file-row.h"
#include "raider-progress-icon.h"

struct _RaiderFileRow
{
   AdwActionRow parent;

  GtkButton* progress_button;
  RaiderProgressIcon* icon;
};

G_DEFINE_TYPE (RaiderFileRow, raider_file_row, ADW_TYPE_ACTION_ROW)


static void
raider_file_row_init (RaiderFileRow *row)
{
  gtk_widget_init_template(GTK_WIDGET(row));

  /* Create the progress icon for the file row. */
  row->icon = g_object_new(RAIDER_TYPE_PROGRESS_ICON, NULL);
  raider_progress_icon_set_progress(row->icon, 0.5);

  gtk_button_set_child(row->progress_button, row->icon);
}

static void
raider_file_row_class_init (RaiderFileRowClass *klass)
{
  gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/Raider/raider-file-row.ui");
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), RaiderFileRow, progress_button);
}

