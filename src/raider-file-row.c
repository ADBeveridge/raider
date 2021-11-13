#include "raider-file-row.h"

G_DEFINE_TYPE (RaiderFileRow, raider_file_row, GTK_TYPE_LIST_BOX_ROW)

static void
raider_file_row_init (RaiderFileRow *row)
{
    row->row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    row->filename_label = gtk_label_new(NULL);
    gtk_container_add(GTK_CONTAINER(row->row_box), row->filename_label);


    row->remove_from_list_button = gtk_button_new_from_icon_name("edit-delete-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(row->row_box), row->remove_from_list_button);

    gtk_container_add (GTK_CONTAINER (row), row->row_box);
}

static void
raider_file_row_dispose (GObject *obj)
{
    G_OBJECT_CLASS (raider_file_row_parent_class)->dispose (obj);
}

static void
raider_file_row_class_init (RaiderFileRowClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = raider_file_row_dispose;
}

RaiderFileRow *raider_file_row_new (const char *str)
{
    RaiderFileRow *file_row = g_object_new (raider_file_row_get_type (), NULL);

    /* Make last-minute additions. */
    gtk_label_set_label(GTK_LABEL(file_row->filename_label), str);
    gtk_widget_show_all (GTK_WIDGET(file_row));

    return file_row;
}
