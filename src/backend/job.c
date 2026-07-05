#define _DEFAULT_SOURCE

#include "utility.h"
#include "raider-file-item.h"
#include <linux/magic.h>
#include <stdio.h>
#include <unistd.h>

typedef struct
{
    RaiderFileItem *item;
    GCancellable *cancel;
    strategy *strat;

    double bytes_written;
    double total_bytes;
    int last_percent;
} Job;

// Shredding engine.
static bool corrupt_pass(Job *job, const char *filename, off_t file_size)
{
    FILE *fp = fopen(filename, "r+");
    if (fp == NULL)
    {
        return false;
    }

    // Fill our writing buffer with the pattern.
    const size_t buf_size = 65536; // 64 KB buffer
    char buffer[buf_size];
    for (size_t i = 0; i < buf_size; i++)
    {
        buffer[i] = job->strat->pattern[i % job->strat->pattern_len];
    }

    off_t bytes_written = 0;
    while (bytes_written < file_size)
    {
        if (g_cancellable_is_cancelled(job->cancel))
        {
            fclose(fp);
            return false;
        }

        size_t remaining = (size_t)(file_size - bytes_written);
        size_t chunk = remaining < buf_size ? remaining : buf_size;

        // Obfuscates the file.
        size_t res = fwrite(buffer, sizeof(char), chunk, fp);
        if (res != chunk)
        {
            fclose(fp);
            g_printerr("Failed to write data to file.\n");
            return false;
        }

        bytes_written += chunk;
        job->bytes_written += chunk;

        int current_percent = (int)((job->bytes_written / job->total_bytes) * 100.0);
        if (current_percent >= job->last_percent + 1)
        {
            raider_file_item_set_progress_async(job->item, job->bytes_written / job->total_bytes);
            job->last_percent = current_percent;
        }
    }

    // Sync our file changes, and close the file.
    if (fflush(fp) != 0)
    {
        g_printerr("Failed to flush C buffer to kernel.\n");
        fclose(fp);
        return false;
    }
    if (fsync(fileno(fp)) != 0)
    {
        g_printerr("Failed to sync data to physical disk.\n");
        fclose(fp);
        return false;
    }
    fclose(fp);
    return true;
}

static bool corrupt_verify(Job *job, const char *filename, off_t file_size)
{
    // TODO: Make sure the file contains the last written data.
    return true;
}

static bool corrupt_unlink(Job *job, const char *filename)
{
    // TODO: Remove the file here.
    return true;
}

static bool corrupt_file_internal(Job *job, const char *filename, off_t file_size)
{
    if (job->strat->pattern_len == 0)
    {
        return true;
    }
    if (job->strat->warning != NULL)
    {
        g_printerr("%s\n", job->strat->warning);
    }

    for (int i = 0; i < job->strat->passes; i++)
    {
        if (corrupt_pass(job, filename, file_size) == false)
        {
            g_printerr("Unable to complete shredding step!\n");
            return false;
        }
    }

    if (remove(filename) != 0)
    {
        g_printerr("Failed to unlink file!\n");
        return false;
    }

    return true;
}

// Shred single file.
bool corrupt_file(RaiderFileItem *item, strategy *strat, GCancellable *cancel)
{
    // Check file.
    const char *filename = raider_file_item_get_path(item);
    if (filename == NULL || !check_file(filename))
    {
        g_printerr("File checks failed!\n");
        return false;
    }

    // Get file size.
    struct stat st;
    if (lstat(filename, &st) != 0)
    {
        g_printerr("Unable to query file information!\n");
        return false;
    }
    off_t size = st.st_size;

    Job job = {
        .item = item,
        .cancel = cancel,
        .strat = strat,
        .bytes_written = 0.0,
        .total_bytes = (double)size * strat->passes,
        .last_percent = -1};

    return corrupt_file_internal(&job, filename, size);
}

static bool shred_folder_recursive(Job *job, GFile *folder)
{
    GError *error = NULL;
    GFileEnumerator *enumerator = g_file_enumerate_children(folder, G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, job->cancel, &error);
    if (enumerator == NULL)
    {
        if (error != NULL)
        {
            g_error_free(error);
        }
        return false;
    }

    GFileInfo *info;
    while ((info = g_file_enumerator_next_file(enumerator, job->cancel, &error)) != NULL)
    {
        if (g_cancellable_is_cancelled(job->cancel))
        {
            g_object_unref(info);
            break;
        }

        GFileType type = g_file_info_get_file_type(info);
        GFile *child = g_file_enumerator_get_child(enumerator, info);

        if (type == G_FILE_TYPE_REGULAR)
        {
            char *child_path = g_file_get_path(child);
            off_t child_size = g_file_info_get_size(info);

            corrupt_file_internal(job, child_path, child_size);

            g_free(child_path);
        }
        else if (type == G_FILE_TYPE_DIRECTORY)
        {
            shred_folder_recursive(job, child);
        }

        g_object_unref(child);
        g_object_unref(info);
    }
    g_object_unref(enumerator);

    if (error != NULL)
    {
        g_error_free(error);
    }

    char *folder_path = g_file_get_path(folder);
    if (folder_path)
    {
        rmdir(folder_path);
        g_free(folder_path);
    }

    return true;
}

bool corrupt_folder(RaiderFileItem *item, strategy *strat, GCancellable *cancel)
{
    GFile *base_folder = raider_file_item_get_file(item);

    off_t total_size = get_folder_size_recursive(base_folder);
    if (total_size == 0)
    {
        char *folder_path = g_file_get_path(base_folder);
        if (folder_path)
        {
            rmdir(folder_path);
            g_free(folder_path);
        }
        raider_file_item_set_progress_async(item, 1.0);
        return true;
    }

    Job job = {
        .item = item,
        .cancel = cancel,
        .strat = strat,
        .bytes_written = 0.0,
        .total_bytes = (double)total_size * strat->passes,
        .last_percent = -1};

    return shred_folder_recursive(&job, base_folder);
}
