/* raider-shred-backend.h
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
#define RAIDER_TYPE_SHRED_BACKEND (raider_shred_backend_get_type())

G_DECLARE_FINAL_TYPE(RaiderShredBackend, raider_shred_backend, RAIDER, SHRED_BACKEND, GObject)

gdouble raider_shred_backend_get_progress (RaiderShredBackend* backend);
