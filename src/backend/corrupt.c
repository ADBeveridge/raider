#define _DEFAULT_SOURCE

#include "corrupt.h"
#include "bucket.h"
#include "utility.h"
#include <glib.h>
#include <stdio.h>

struct _Corrupt
{
    GObject parent;

    GList *buckets;
    GCancellable *cancel;
    GListStore *store;
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
    Corrupt *self = g_object_new(corrupt_get_type(), NULL);
    self->store = g_list_store_new(RAIDER_TYPE_FILE_ITEM); // Initialize it
    return self;
}

GListModel *corrupt_get_model(Corrupt *self)
{
    return G_LIST_MODEL(self->store);
}

void corrupt_clear_files(Corrupt *self)
{
    g_list_store_remove_all(self->store);
}

void corrupt_remove_file(Corrupt *self, RaiderFileItem *item)
{
    guint n_items = g_list_model_get_n_items(G_LIST_MODEL(self->store));
    for (guint i = 0; i < n_items; i++) {
        RaiderFileItem *existing = g_list_model_get_item(G_LIST_MODEL(self->store), i);
        if (existing == item) {
            g_list_store_remove(self->store, i);
            g_object_unref(existing);
            break;
        }
        g_object_unref(existing);
    }
}

void corrupt_add_file(Corrupt *self, GFile *file)
{
    RaiderFileItem *item = raider_file_item_new(file);
    g_signal_connect_swapped(item, "shred-finished", G_CALLBACK(corrupt_remove_file), self);
    g_list_store_append(self->store, item);
    g_object_unref(item);
}

static void corrupt_add_to_bucket(Corrupt *self, RaiderFileItem *item)
{
    char *filename = raider_file_item_get_path(item);
    if (filename == NULL)
    {
        return;
    };

    // Get the id of the device the file resides on.
    struct stat st;
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
            bucket_add_file(bucket, item);
            return;
        }
    }

    // If not, create a new bucket.
    strategy strat = getStrategy(filename);

    Bucket *new_bucket = g_new0(Bucket, 1);
    new_bucket->deviceID = id;
    new_bucket->strategy = strat;
    new_bucket->cancel = self->cancel;

    bucket_add_file(new_bucket, item);
    self->buckets = g_list_append(self->buckets, new_bucket);
}

void master_bucket_worker (gpointer data, gpointer user_data)
{
    bucket_shred(data);
}

// Runs in its own thread.
void shred_all_task_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    Corrupt *self = (Corrupt *)source_object;
    self->cancel = cancellable;
    GError *error = NULL;

    // Create the thread pool.
    gint max_concurrent_drives = g_get_num_processors();
    GThreadPool *master_pool = g_thread_pool_new(master_bucket_worker, NULL, max_concurrent_drives, FALSE, &error);
    if (error != NULL)
    {
        g_task_return_error(task, error);
        return;
    }

    // Sort files and folders into buckets.
    guint n_items = g_list_model_get_n_items(G_LIST_MODEL(self->store));
    for (guint i = 0; i < n_items; i++)
    {
        RaiderFileItem *item = g_list_model_get_item(G_LIST_MODEL(self->store), i);

        corrupt_add_to_bucket(self, item);

        g_object_unref(item);
    }

    // Call master_bucket_worker with all the buckets in different threads.
    for (GList *l = self->buckets; l != NULL; l = l->next)
    {
        g_thread_pool_push(master_pool, l->data, &error);

        if (error != NULL)
        {
            g_printerr("Failed to push bucket to thread pool: %s\n", error->message);
            g_clear_error(&error);
        }
    }

    // Block till all threads finish and free memory.
    g_thread_pool_free(master_pool, FALSE, TRUE);

    // Delete all buckets to reset backend.
    for (GList *l = self->buckets; l != NULL; l = l->next)
    {
        Bucket *bucket = (Bucket *)l->data;
        g_list_free(bucket->files);
        g_free(bucket);
    }
    g_list_free(self->buckets);
    self->buckets = NULL;

    g_task_return_boolean(task, TRUE);
}

// The paired finish function to retrieve the result in your callback
gboolean corrupt_start_shredding_finish(Corrupt *self, GAsyncResult *res, GError **error)
{
    g_return_val_if_fail(g_task_is_valid(res, self), FALSE);
    return g_task_propagate_boolean(G_TASK(res), error);
}
