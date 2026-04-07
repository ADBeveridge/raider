#ifndef CORRUPT_BUCKET_H
#define CORRUPT_BUCKET_H

#include "utility.h"
#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>

typedef struct Bucket
{
    GList *files;
    dev_t deviceID;
    struct strategy strategy;
} Bucket;

bool bucket_add_file(Bucket *self, const char *filename);
void bucket_shred(Bucket *self, GCancellable *cancel);

#endif // CORRUPT_BUCKET_H
