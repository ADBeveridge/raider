#include "raider-file-row.h"

G_DEFINE_TYPE (RaiderFileRow, raider_file_row, GTK_TYPE_LIST_BOX_ROW)

void raider_file_row_delete (GtkWidget *widget, gpointer data)
{
    GtkWidget *widget2 = gtk_widget_get_parent(widget);
    GtkWidget *widget3 = gtk_widget_get_parent(widget2);
    gtk_widget_destroy(widget3);
}

static void
raider_file_row_init (RaiderFileRow *row)
{
    row->row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    row->filename_label = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(row->row_box), row->filename_label, TRUE, TRUE, 0);
    gtk_widget_set_halign(row->filename_label, GTK_ALIGN_START);

    row->remove_from_list_button = gtk_button_new_from_icon_name("edit-delete-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(row->row_box), row->remove_from_list_button, TRUE, TRUE, 0);
    gtk_widget_set_halign(row->remove_from_list_button, GTK_ALIGN_END);
    g_signal_connect(row->remove_from_list_button, "clicked", G_CALLBACK(raider_file_row_delete), NULL);

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
