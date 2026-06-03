#ifndef CORRUPT_BUCKET_H
#define CORRUPT_BUCKET_H

#include "utility.h"
#include "raider-file-item.h"
#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>

typedef struct _FilePayload FilePayload;

typedef struct Bucket
{
    GList *files;
    dev_t deviceID;
    struct strategy strategy;
} Bucket;

void bucket_add_file(Bucket *self, FilePayload *fp);
void bucket_shred(Bucket *self, GCancellable *cancel);

#endif // CORRUPT_BUCKET_H
