/* raider-application.h
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

#include <adwaita.h>

G_BEGIN_DECLS

#define RAIDER_TYPE_APPLICATION (raider_application_get_type())

G_DECLARE_FINAL_TYPE (RaiderApplication, raider_application, RAIDER, APPLICATION, AdwApplication)

RaiderApplication *raider_application_new (gchar *application_id,
                                           GApplicationFlags  flags);

G_END_DECLS
