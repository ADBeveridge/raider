#pragma once
#include <gtk/gtk.h>
#include <adwaita.h>
#define RAIDER_TYPE_FILE_ITEM (raider_file_item_get_type())

G_DECLARE_FINAL_TYPE(RaiderFileItem, raider_file_item, RAIDER, FILE_ITEM, GObject)

void raider_file_item_set_started_async(RaiderFileItem *self);
void raider_file_item_set_progress_async(RaiderFileItem *self, double progress);
void raider_file_item_set_finished_async(RaiderFileItem *self);

double raider_file_item_get_progress(RaiderFileItem *item);
GFile *raider_file_item_get_file(RaiderFileItem *item);
gchar *raider_file_item_get_name(RaiderFileItem *item);
gchar *raider_file_item_get_path(RaiderFileItem *item);
gboolean raider_file_item_is_folder(RaiderFileItem *item);
RaiderFileItem *raider_file_item_new(GFile *file);
