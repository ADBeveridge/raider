#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RAIDER_TYPE_PROGRESS_ICON (raider_progress_icon_get_type())

G_DECLARE_FINAL_TYPE (RaiderProgressIcon, raider_progress_icon, RAIDER, PROGRESS_ICON, GtkDrawingArea)

gdouble raider_progress_icon_get_progress (RaiderProgressIcon *self);
void raider_progress_icon_set_progress (RaiderProgressIcon *self, gdouble progress);

G_END_DECLS
