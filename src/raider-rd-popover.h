#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>


#define RAIDER_TYPE_RD_POPOVER (raider_rd_popover_get_type ())
G_DECLARE_FINAL_TYPE (RaiderRdPopover, raider_rd_popover, RAIDER, RD_POPOVER, GtkPopover)

