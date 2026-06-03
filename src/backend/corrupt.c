#define _DEFAULT_SOURCE

#include "corrupt.h"
#include "bucket.h"
#include "utility.h"
#include <glib.h>
#include <stdio.h>

static void file_progress_free(gpointer data)
{
    FilePayload *fp = (FilePayload *)data;
    g_object_unref(fp->file);
    g_free(fp);
}

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

void corrupt_clear_files(Corrupt *self) {
    g_list_store_remove_all(self->store);
}

void corrupt_remove_file(Corrupt *self, RaiderFileItem *item) {
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
    g_list_store_append(self->store, item);
    g_object_unref(item);
}

static void corrupt_add_to_bucket(Corrupt *self, FilePayload *fp)
{
    char *filename = g_file_get_path(fp->file);
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
            bucket_add_file(bucket, fp);

            g_free(filename);

            return;
        }
    }

    // If not, create a new bucket.
    strategy strat = getStrategy(filename);

    Bucket *new_bucket = g_new0(Bucket, 1);
    new_bucket->deviceID = id;
    new_bucket->strategy = strat;

    bucket_add_file(new_bucket, fp);
    self->buckets = g_list_append(self->buckets, new_bucket);

    g_free(filename);
}

// Recursively extracts regular files from directories.
static void corrupt_scan_folder_recursive(GFile *target, RaiderFileItem *item, GList **file_list)
{
    GError *error = NULL;
    GFileInfo *info = g_file_query_info(target, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &error);

    if (error != NULL)
    {
        g_printerr("Skipping unreadable path: %s\n", error->message);
        g_error_free(error);
        return;
    }

    GFileType file_type = g_file_info_get_file_type(info);

    // If it is a standard file, add it to our master list
    if (file_type == G_FILE_TYPE_REGULAR)
    {
        FilePayload *extracted_target = g_new(FilePayload, 1);
        extracted_target->file = g_object_ref(target);
        extracted_target->item = item;

        *file_list = g_list_prepend(*file_list, extracted_target);
    }
    // If it is a directory recurse
    else if (file_type == G_FILE_TYPE_DIRECTORY)
    {
        GFileEnumerator *enumerator = g_file_enumerate_children(target, G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &error);
        if (enumerator != NULL)
        {
            GFileInfo *child_info;
            while ((child_info = g_file_enumerator_next_file(enumerator, NULL, &error)) != NULL)
            {
                GFile *child = g_file_enumerator_get_child(enumerator, child_info);
                corrupt_scan_folder_recursive(child, item, file_list);

                g_object_unref(child);
                g_object_unref(child_info);
            }
            g_object_unref(enumerator);
        }

        if (error != NULL)
        {
            g_error_free(error); // Clear enumerator errors (e.g. permission denied on a subfolder)
        }
    }

    g_object_unref(info);
}

void master_bucket_worker (gpointer data, gpointer user_data)
{
    bucket_shred(data, user_data);
}

static void shred_all_task_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    Corrupt *self = (Corrupt *)source_object;
    GError *error = NULL;
    GList *master_file_list = NULL;

    guint n_items = g_list_model_get_n_items(G_LIST_MODEL(self->store));

    // Expand all folders into many files.
    for (guint i = 0; i < n_items; i++)
    {
        RaiderFileItem *item = g_list_model_get_item(G_LIST_MODEL(self->store), i);
        GFile *file = raider_file_item_get_file(item);

        corrupt_scan_folder_recursive(file, item, &master_file_list);

        g_object_unref(item);
    }

    // Sort into buckets.
    for (GList *l = master_file_list; l != NULL; l = l->next)
    {
        FilePayload *fp = (FilePayload *)l->data;
        corrupt_add_to_bucket(self, fp);
    }

    g_list_free_full(master_file_list, file_progress_free);

    // Create the thread pool.
    gint max_concurrent_drives = g_get_num_processors();
    GThreadPool *master_pool = g_thread_pool_new(master_bucket_worker, cancellable, max_concurrent_drives, FALSE, &error);

    if (error != NULL)
    {
        g_task_return_error(task, error);
        return;
    }

    // Push the buckets into the pool
    for (GList *l = self->buckets; l != NULL; l = l->next)
    {
        g_thread_pool_push(master_pool, l->data, &error);

        if (error != NULL)
        {
            g_printerr("Failed to push bucket to thread pool: %s\n", error->message);
            g_clear_error(&error);
        }
    }

    // Block till all threads finish.
    g_thread_pool_free(master_pool, FALSE, TRUE);

    // Delete all buckets to reset backend.
    for (GList *l = self->buckets; l != NULL; l = l->next)
    {
        g_free(l->data);
    }
    g_list_free(self->buckets);
    self->buckets = NULL;

    g_task_return_boolean(task, TRUE);
}

// The asynchronous entry point called from the main UI
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
