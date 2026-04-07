/* raider-progress-info-popover.c
 *
 * Copyright 2022 Alan Beveridge
 *
 * raider is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * raider is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "corrupt.h"
#include "utility.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

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

bool corrupt_add_file(Corrupt *self, const char *filename)
{
    bool result = check_file(name.c_str());
    if (result == false)
    {
        return false;
    }

    addToBucket(name);

    return true;
}

static void corrupt_add_to_bucket(Corrupt *self, const char *filename)
{
    struct stat st;

    // Get the id of the device the file resides on.
    if (lstat(name.c_str(), &st) != 0)
    {
        fprintf("Error getting device id of file.\n");
        return;
    }
    dev_t id = st.st_dev;

    // Check to see if we already created a bucket for this file.
    int length = g_list_length(self->buckets);
    for (int i = 0; i < length; i++)
    {
        bucket *bucket = g_list_nth_data(self->buckets, i);
        if (bucket_get_device_id(bucket) == id)
        {
            bucket_add_file(bucket, filename);
            return;
        }
    }

    // If not, create a new bucket.
    strategy strat = getStrategy(filename);

    bucket *bucket = malloc(sizeof(bucket));
    bucket->deviceID = id;
    bucket->strategy = strat;

    g_list_append(self->buckets, bucket);
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
    shred(bucket, cancel);
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
            g_error_clear(&error);
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
