/*
 * Copyright © 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file has been copied over from Nautilus.
 */

#pragma once
#include <gtk/gtk.h>
#define RAIDER_TYPE_PROGRESS_PAINTABLE (raider_progress_paintable_get_type())

G_DECLARE_FINAL_TYPE(RaiderProgressPaintable, raider_progress_paintable, RAIDER, PROGRESS_PAINTABLE, GObject)

GdkPaintable *raider_progress_paintable_new (GtkWidget *widget);
void raider_progress_paintable_animate_done (RaiderProgressPaintable *self);

