#ifndef __RAIDERFILEROW_H
#define __RAIDERFILEROW_H

#include <gtk/gtk.h>
#include <handy.h>
#include "raider.h"

#define RAIDER_FILE_ROW_TYPE (raider_file_row_get_type ())

G_DECLARE_FINAL_TYPE (RaiderFileRow, raider_file_row, RAIDER, FILE_ROW, HdyActionRow)

RaiderFileRow *raider_file_row_new (const char *str);
void launch (gpointer data, gpointer user_data);

#endif
