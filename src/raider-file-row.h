#pragma once

#include <gtk/gtk.h>

#define RAIDER_FILE_ROW_TYPE (raider_file_row_get_type ())

G_DECLARE_FINAL_TYPE (RaiderFileRow, raider_file_row, RAIDER, FILE_ROW, AdwActionRow)

