/* raider-window.h
 *
 * Copyright 2022 Alan Beveridge
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

#define RAIDER_TYPE_WINDOW (raider_window_get_type())

G_DECLARE_FINAL_TYPE(RaiderWindow, raider_window, RAIDER, WINDOW, AdwApplicationWindow)

gboolean raider_window_open_file(GFile *file, gpointer data, gchar *title);
void raider_window_close_file(gpointer data, gpointer user_data, gint result);
void raider_window_show_toast(RaiderWindow *window, gchar *text);
void raider_window_open_files(RaiderWindow *window, GList *file_list);
gboolean raider_window_exit(RaiderWindow *window, gpointer data);
