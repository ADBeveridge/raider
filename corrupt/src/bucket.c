#include "bucket.h"
#include "utility.h"

bool add_file(Bucket *self, const char *filename)
{
    g_list_append(self->files, filename); // TODO: Make sure that we don't need to duplicate our string.
    return true;
}

// A custom context to pass shared data to our worker threads
typedef struct
{
    Bucket *bucket;
    GCancellable *cancel;
} WorkerContext;

// The function executed by the threads for each pushed file
static void worker_thread(gpointer data, gpointer user_data)
{
    const char *filename = (const char *)data;
    WorkerContext *context = (WorkerContext *)user_data;

    corrupt_file(filename, context->bucket->strategy);
}

void shred(Bucket *self, GCancellable *cancel)
{
    GError *error = NULL;

    WorkerContext *context = g_new(WorkerContext, 1);
    context->bucket = self;
    context->cancel = cancel;

    GThreadPool *pool = g_thread_pool_new(worker_thread, context, self->strategy.thread_count, TRUE, &error);
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
            g_error_clear(&error);
        }
    }

    // 3. Cleanup and wait for completion
    g_thread_pool_free(pool, FALSE, TRUE);

    g_free(context);
}
