#ifndef CORRUPT_BUCKET_H
#define CORRUPT_BUCKET_H

#include "utility.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

typedef struct Bucket
{
    GList *files;
    dev_t deviceID;
    struct strategy strategy;
} Bucket;

bool add_file(Bucket *self, const char *filename);
void shred(Bucket *self, GCancellable *cancel);

#endif // CORRUPT_BUCKET_H
