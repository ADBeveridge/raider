#pragma once

#include <time.h> /* time */
#include <stdio.h> /* FILE, fopen, fwrite, fclose, fprintf */
#include <stdint.h> /* uint*_t int*_t */
#include <string.h> /* strlen */
#include <stdlib.h> /* srand, rand */
#include <sys/stat.h> /* stat */
#include <sys/types.h> /* off_t */
#include <gtk/gtk.h> /* progress variables */
#include "raider-file-row.h"
#include "raider-progress-info-popover.h"
#include "raider-progress-paintable.h"

#define RAIDER_CORRUPT_TYPE (raider_corrupt_get_type())

G_DECLARE_FINAL_TYPE(RaiderCorrupt, raider_corrupt, RAIDER, CORRUPT, GObject)

RaiderCorrupt *raider_corrupt_new(GFile* file, RaiderFileRow* row);
GCancellable* raider_corrupt_start_shredding(RaiderCorrupt* self, GAsyncReadyCallback func);



