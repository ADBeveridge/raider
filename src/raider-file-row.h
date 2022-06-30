#pragma once

#include <gtk/gtk.h>

#define RAIDER_TYPE_FILE_ROW (raider_file_row_get_type ())

G_DECLARE_FINAL_TYPE (RaiderFileRow, raider_file_row, RAIDER, FILE_ROW, AdwActionRow)

RaiderFileRow *raider_file_row_new (GFile* file);
gchar* raider_file_row_get_filename(RaiderFileRow* row);
void launch_shredding(gpointer data);
