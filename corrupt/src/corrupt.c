#define _DEFAULT_SOURCE

#include "corrupt.h"
#include "utility.h"
#include "bucket.h"
#include <glib.h>
#include <stdio.h>

struct _Corrupt
{
    GObject parent;

    GList *buckets;
    GCancellable *cancel;
};

G_DEFINE_TYPE(Corrupt, corrupt, G_TYPE_OBJECT)

static void corrupt_init(Corrupt *self)
{
}

static void corrupt_class_init(CorruptClass *klass)
{
}

Corrupt *corrupt_new()
{
    Corrupt *corrupt = g_object_new(corrupt_get_type(), NULL);
    return corrupt;
}

static void corrupt_add_to_bucket(Corrupt *self, const char *filename)
{
    struct stat st;

    // Get the id of the device the file resides on.
    if (lstat(filename, &st) != 0)
    {
        fprintf(stderr, "Error getting device id of file.\n");
        return;
    }
    dev_t id = st.st_dev;

    // Check to see if we already created a bucket for this file.
    int length = g_list_length(self->buckets);
    for (int i = 0; i < length; i++)
    {
        Bucket *bucket = g_list_nth_data(self->buckets, i);
        if (bucket->deviceID == id)
        {
            bucket_add_file(bucket, filename);
            return;
        }
    }

    // If not, create a new bucket.
    strategy strat = getStrategy(filename);

    Bucket *new_bucket = g_new0(Bucket, 1);
    new_bucket->deviceID = id;
    new_bucket->strategy = strat;

    self->buckets = g_list_append(self->buckets, new_bucket);
}

bool corrupt_add_file(Corrupt *self, const char *filename)
{
    bool result = check_file(filename);
    if (result == false)
    {
        return false;
    }

    corrupt_add_to_bucket(self, filename);

    return true;
}

static void master_bucket_worker(gpointer data, gpointer user_data)
{
    Bucket *bucket = (Bucket *)data;
    GCancellable *cancel = (GCancellable *)user_data;

    if (g_cancellable_is_cancelled(cancel))
    {
        return;
    }

    // This blocks this thread.
    bucket_shred(bucket, cancel);
}

static void shred_all_task_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    Corrupt *self = (Corrupt *)source_object;
    GError *error = NULL;

    // Determine a dynamic limit for concurrent device I/O.
    // Tying this to the processor count is a solid heuristic to avoid OS thrashing.
    gint max_concurrent_drives = g_get_num_processors();

    // 1. Initialize the master pool
    GThreadPool *master_pool = g_thread_pool_new(master_bucket_worker, cancellable, max_concurrent_drives, FALSE, &error);

    if (error != NULL)
    {
        g_task_return_error(task, error);
        return;
    }

    // Wait for all buckets.
    for (GList *l = self->buckets; l != NULL; l = l->next)
    {
        g_thread_pool_push(master_pool, l->data, &error);
        if (error != NULL)
        {
            g_printerr("Failed to push bucket to master pool: %s\n", error->message);
            g_clear_error(&error);
        }
    }

    // Wait for all the buckets to finish.
    g_thread_pool_free(master_pool, FALSE, TRUE);
    g_task_return_boolean(task, TRUE);
}

// The asynchronous entry point called from your main UI
void corrupt_start_shredding_async(Corrupt *self, GCancellable *cancel, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task = g_task_new(self, cancel, callback, user_data);
    g_task_run_in_thread(task, shred_all_task_thread);
    g_object_unref(task);
}

// The paired finish function to retrieve the result in your callback
gboolean corrupt_start_shredding_finish(Corrupt *self, GAsyncResult *res, GError **error)
{
    g_return_val_if_fail(g_task_is_valid(res, self), FALSE);
    return g_task_propagate_boolean(G_TASK(res), error);
}
