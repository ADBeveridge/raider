#include <gtk/gtk.h>
#include "raider-file-list.h"

struct _RaiderFileList {
  GtkListBox parent;
};

G_DEFINE_TYPE(RaiderFileList, raider_file_list, GTK_TYPE_LIST_BOX)

static void raider_file_list_class_init(RaiderFileListClass *klass)
{
}

static void
raider_file_list_init(RaiderFileList *row)
{
  //gtk_widget_init_template(GTK_WIDGET(self));
}


