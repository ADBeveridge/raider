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

// Recursively extracts regular files from directories.
static void corrupt_scan_folder_recursive(GFile *target, GList **file_list)
{
    GError *error = NULL;
    GFileInfo *info = g_file_query_info(target,
                                        G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                        NULL,
                                        &error);

    if (error != NULL) {
        g_printerr("Skipping unreadable path: %s\n", error->message);
        g_error_free(error);
        return;
    }

    GFileType file_type = g_file_info_get_file_type(info);

    // If it is a standard file, add it to our master list
    if (file_type == G_FILE_TYPE_REGULAR)
    {
        *file_list = g_list_prepend(*file_list, g_object_ref(target));
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
                corrupt_scan_folder_recursive(child, file_list);

                g_object_unref(child);
                g_object_unref(child_info);
            }
            g_object_unref(enumerator);
        }

        if (error != NULL) {
            g_error_free(error); // Clear enumerator errors (e.g. permission denied on a subfolder)
        }
    }

    g_object_unref(info);
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

    GList *input_files = (GList *)task_data;
    GList *master_file_list = NULL;

    // If any file is a folder, scan the files.
    for (GList *l = input_files; l != NULL; l = l->next) {
        corrupt_scan_folder_recursive(G_FILE(l->data), &master_file_list);
    }

    // Add all the files into the buckets.
    for (GList *l = master_file_list; l != NULL; l = l->next) {
        if (g_cancellable_is_cancelled(cancellable)) break;

        char *path = g_file_get_path(G_FILE(l->data));
        if (path != NULL) {
            corrupt_add_to_bucket(self, path);
            g_free(path);
        }
    }

    gint max_concurrent_drives = g_get_num_processors();
    GThreadPool *master_pool = g_thread_pool_new(master_bucket_worker, cancellable, max_concurrent_drives, FALSE, &error);

    if (error != NULL)
    {
        g_task_return_error(task, error);
        return;
    }

    for (GList *l = self->buckets; l != NULL; l = l->next)
    {
        g_thread_pool_push(master_pool, l->data, &error);
        if (error != NULL)
        {
            g_printerr("Failed to push bucket to master pool: %s\n", error->message);
            g_clear_error(&error);
        }
    }

    g_thread_pool_free(master_pool, FALSE, TRUE); // Block till all the buckets finish.
    g_task_return_boolean(task, TRUE);
}

// The asynchronous entry point called from your main UI
void corrupt_start_shredding_async(Corrupt *self, GList *files_to_shred, GCancellable *cancel, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task = g_task_new(self, cancel, callback, user_data);
    g_task_set_task_data(task, files_to_shred, (GDestroyNotify)g_list_free_full);
    g_task_run_in_thread(task, shred_all_task_thread);
    g_object_unref(task);
}

// The paired finish function to retrieve the result in your callback
gboolean corrupt_start_shredding_finish(Corrupt *self, GAsyncResult *res, GError **error)
{
    g_return_val_if_fail(g_task_is_valid(res, self), FALSE);
    return g_task_propagate_boolean(G_TASK(res), error);
}
