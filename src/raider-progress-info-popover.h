#ifndef __RAIDERPROGRESSINFOPOPOVER_H
#define __RAIDERPROGRESSINFOPOPOVER_H

#include <gtk/gtk.h>

#define RAIDER_PROGRESS_INFO_POPOVER_TYPE (raider_progress_info_popover_get_type ())

G_DECLARE_FINAL_TYPE (RaiderProgressInfoPopover, raider_progress_info_popover, RAIDER, PROGRESS_INFO_POPOVER, GtkPopover)

RaiderProgressInfoPopover *raider_file_row_new (const char *str);

#endif
