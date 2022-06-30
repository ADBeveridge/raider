#pragma once

#include <gtk/gtk.h>

#define RAIDER_PROGRESS_INFO_POPOVER_TYPE (raider_progress_info_popover_get_type ())

G_DECLARE_FINAL_TYPE (RaiderProgressInfoPopover, raider_progress_info_popover, RAIDER, PROGRESS_INFO_POPOVER, GtkPopover)

RaiderProgressInfoPopover *raider_progress_info_popover_new (GtkWidget *relative);
void raider_progress_info_popover_set_progress (RaiderProgressInfoPopover *popover, gdouble fraction);
void raider_progress_info_popover_set_text (RaiderProgressInfoPopover *popover, gchar *text);

