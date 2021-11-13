#ifndef __RAIDERFILEROW_H
#define __RAIDERFILEROW_H

#include "raider.h"

#define RAIDER_FILE_ROW_TYPE (raider_file_row_get_type ())

G_DECLARE_FINAL_TYPE (RaiderFileRow, raider_file_row, RAIDER, FILE_ROW, GtkListBoxRow)

struct _RaiderFileRow
{
    GtkListBoxRow parent;

    GtkWidget *row_box;
    GtkWidget *filename_label;
    GtkWidget *remove_from_list_button;

    gint id;
    gchar *filename;
};

RaiderFileRow *raider_file_row_new (const char *str);

#endif
