#ifndef CORRUPT_JOB_H
#define CORRUPT_JOB_H

#include <gio/gio.h>
#include "raider-file-item.h"

bool corrupt_file(RaiderFileItem *item, strategy *strat, GCancellable *cancel);
bool corrupt_folder(RaiderFileItem *item, strategy *strat, GCancellable *cancel);

#endif // CORRUPT_JOB_H