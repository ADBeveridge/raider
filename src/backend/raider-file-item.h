/* raider-file-item.h
 *
 * Copyright 2026 Alan Beveridge
 *
 * raider is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * raider is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <gtk/gtk.h>
#include <adwaita.h>
#define RAIDER_TYPE_FILE_ITEM (raider_file_item_get_type())

G_DECLARE_FINAL_TYPE(RaiderFileItem, raider_file_item, RAIDER, FILE_ITEM, GObject)

double raider_file_item_get_progress(RaiderFileItem *item);
void raider_file_item_set_progress_safe(RaiderFileItem *self, double progress);
GFile *raider_file_item_get_file(RaiderFileItem *item);
gchar *raider_file_item_get_name(RaiderFileItem *item);
gchar *raider_file_item_get_path(RaiderFileItem *item);
gboolean raider_file_item_is_folder(RaiderFileItem *item);
RaiderFileItem *raider_file_item_new(GFile *file);

