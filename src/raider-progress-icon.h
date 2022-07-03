/* raider-progress-icon.h
 *
 * Copyright 2022 Alan Beveridge
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
#define RAIDER_TYPE_PROGRESS_ICON (raider_progress_icon_get_type())

G_DECLARE_FINAL_TYPE(RaiderProgressIcon, raider_progress_icon, RAIDER, PROGRESS_ICON, GtkDrawingArea)

gdouble raider_progress_icon_get_progress(RaiderProgressIcon *self);
void raider_progress_icon_set_progress(RaiderProgressIcon *self, gdouble progress);
