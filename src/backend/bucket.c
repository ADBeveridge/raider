#include "bucket.h"
#include "utility.h"

typedef struct
{
    Bucket *bucket;
    GCancellable *cancel;
} FileContext;

void bucket_add_file(Bucket *self, FilePayload *payload)
{
    self->files = g_list_append(self->files, payload);
}

// The function executed by the threads for each pushed file
static void file_shred(gpointer data, gpointer user_data)
{
    FilePayload *payload = (FilePayload *)data;
    FileContext *context = (FileContext *)user_data;

    // Shred the file!
    corrupt_file(payload, &context->bucket->strategy);
}

// Uses a GThreadPool within GThreadPool.
void bucket_shred(Bucket *self, GCancellable *cancel)
{
    GError *error = NULL;

    FileContext *context = g_new(FileContext, 1);
    context->bucket = self;
    context->cancel = cancel;

    GThreadPool *pool = g_thread_pool_new(file_shred, context, self->strategy.thread_count, TRUE, &error);
    if (error != NULL)
    {
        g_printerr("Failed to create thread pool: %s\n", error->message);
        g_error_free(error);
        g_free(context);
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
    g_free(context);
}
