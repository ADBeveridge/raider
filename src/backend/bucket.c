#include "bucket.h"
#include "utility.h"
#include "job.h"
#include "raider-file-item.h"
#include "shred-manager.h"

void bucket_add_file(Bucket *self, RaiderFileItem *item)
{
    // TODO: Ref item for our own use.
    self->files = g_list_append(self->files, item);
}

// The function executed by the threads for each pushed file
static void file_shred(gpointer data, gpointer user_data)
{
    RaiderFileItem *item = (RaiderFileItem *)data;
    Bucket *bucket = (Bucket *)user_data;

    raider_file_item_set_started_async(item);

    bool res;
    if (raider_file_item_is_folder(item))
    {
        res = corrupt_folder(item, &bucket->strategy, bucket->cancel);
    }
    else
    {
        res = corrupt_file(item, &bucket->strategy, bucket->cancel);
    }

    if (!res)
    {
        g_printerr("File shredding failed!\n");
        return;
    }

    raider_file_item_set_finished_async(item);
}

// Uses a GThreadPool within GThreadPool.
void bucket_shred(Bucket *self)
{
    GError *error = NULL;

    GThreadPool *pool = g_thread_pool_new(file_shred, self, self->strategy.thread_count, TRUE, &error);
    if (error != NULL)
    {
        g_printerr("Failed to create thread pool: %s\n", error->message);
        g_error_free(error);
        return;
    }

    for (GList *l = self->files; l != NULL; l = l->next)
    {
        g_thread_pool_push(pool, l->data, &error);
        if (error != NULL)
        {
            g_printerr("Failed to push to pool: %s\n", error->message);
            g_clear_error(&error);
        }
    }

    // Cleanup and wait for completion.
    g_thread_pool_free(pool, FALSE, TRUE);
}
