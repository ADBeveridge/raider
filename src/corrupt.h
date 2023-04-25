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
#include "raider-progress-icon.h"

struct _corrupt_data
{
    RaiderProgressInfoPopover* popover;
    RaiderProgressIcon* icon;
    GTask* task;
    double progress;
    GMutex progress_mutex;
};

uint8_t corrupt_file(const char *filename, struct _corrupt_data *corrupt_data);
uint8_t corrupt_unlink_file(const char *filename);

