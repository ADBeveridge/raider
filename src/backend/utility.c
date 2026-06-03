#define _DEFAULT_SOURCE

#include "utility.h"
#include "raider-file-item.h"
#include "corrupt.h"
#include <linux/magic.h>
#include <stdio.h>
#include <unistd.h>

// Smart defaults for common filesystems, if you can think of any changes that would help, please open an issue, I will be in touch.
strategy getStrategy(const char *filename)
{
    struct statfs fs_info;
    if (statfs(filename, &fs_info) != 0)
    {
        fprintf(stderr, "Error getting filesystem info.\n");
        return (strategy){1, "abc", 3, false, false, 1, "Error getting filesystem info"};
    }

    __fsword_t filesystem = fs_info.f_type;

    switch (filesystem)
    {
    case EXT4_SUPER_MAGIC:
        return (strategy){3, "\x00\xFF\x55", 3, false, false, 1, NULL};

    case BTRFS_SUPER_MAGIC:
        return (strategy){1, "0", 1, false, false, 1, "Shredding not guaranteed on a copy-on-write filesystem"};

    default:
        return (strategy){1, "abc", 3, false, false, 1, "File is on an unknown filesystem"};
    }
}

bool check_file(const char *filename)
{
    struct stat st;

    if (lstat(filename, &st) != 0)
    {
        fprintf(stderr, "corrupt: current file not found\n");
        return false;
    }
    if (S_ISLNK(st.st_mode) == 1)
    {
        fprintf(stderr, "corrupt: symbolic link cannot be processed\n");
        return false;
    }
    if (S_ISREG(st.st_mode) == 0)
    {
        fprintf(stderr, "corrupt: current file is not a regular file\n");
        return false;
    }
    return true;
}

static bool corrupt_pass(const char *filename, struct strategy *strat, FilePayload *payload, off_t size, int current_pass)
{
    if (strat->pattern_len == 0)
    {
        return true;
    }

    // Run some checks.
    FILE *fp = fopen(filename, "r+");
    if (fp == NULL)
    {
        return false;
    }

    const size_t buf_size = 65536; // 64 KB buffer
    char buffer[buf_size];

    // Fill the buffer with the pattern.
    for (size_t i = 0; i < buf_size; i++)
    {
        buffer[i] = strat->pattern[i % strat->pattern_len];
    }

    off_t bytes_written = 0;
    int last_reported_percent = -1;

    while (bytes_written < size)
    {
        size_t remaining = (size_t)(size - bytes_written);
        size_t chunk = remaining < buf_size ? remaining : buf_size;

        fwrite(buffer, sizeof(char), chunk, fp);
        bytes_written += chunk;

        // Calculate progress.
        double total_target_bytes = (double)size * strat->passes;
        double overall_bytes_written = ((double)size * current_pass) + bytes_written;

        int current_percent = (int)((overall_bytes_written / total_target_bytes) * 100.0);

        // Ping UI if progress updated by 1%.
        if (current_percent >= last_reported_percent + 1)
        {
            raider_file_item_set_progress_safe(payload->item, overall_bytes_written / total_target_bytes);

            last_reported_percent = current_percent;
        }
    }

    fflush(fp);
    fsync(fileno(fp));
    fclose(fp);
    return true;
}

bool corrupt_file(FilePayload *payload, strategy *strat)
{
    char *filename = g_file_get_path(payload->file);
    if (filename == NULL)
    {
        return false;
    };

    if (!check_file(filename))
    {
        return false;
    }

    // Get filesize.
    struct stat st;
    if (lstat(filename, &st) != 0)
    {
        g_free(filename);
        return false;
    }
    off_t size = st.st_size;

    // Shred the file by overwriting it many times.
    for (int i = 0; i < strat->passes; i++)
    {
        if (corrupt_pass(filename, strat, payload, size, i) == false)
        {
            fprintf(stderr, "corrupt: failed to perform shredding step\n");
            g_free(filename);
            return false;
        }
    }

    if (remove(filename) != 0)
    {
        fprintf(stderr, "corrupt: failed to unlink file\n");
        g_free(filename);
        return false;
    }

    g_free(filename);
    return true;
}
