#define _DEFAULT_SOURCE

#include "utility.h"
#include <linux/magic.h>
#include <stdio.h>
#include <unistd.h>

strategy getStrategy(const char *filename)
{
    struct statfs fs_info;
    if (statfs(filename, &fs_info) != 0)
    {
        fprintf(stderr, "Error getting filesystem info.\n");
        return (strategy){1, "abc", 3, false, 1, "Error getting filesystem info"};
    }

    __fsword_t filesystem = fs_info.f_type;

    switch (filesystem)
    {
    case EXT4_SUPER_MAGIC:
        return (strategy){3, "\x00\xFF\x55", 3, false, 1, NULL};

    case BTRFS_SUPER_MAGIC:
        return (strategy){1, "0", 1, false, 1, "Shredding not guaranteed on a copy-on-write filesystem"};

    default:
        g_print("Detected Filesystem Magic: 0x%lX\n", (unsigned long)filesystem);
        return (strategy){1, "abc", 3, false, 1, "File is on an unknown filesystem"};
    }
}

bool check_file(const char *filename)
{
    struct stat st;
    if (lstat(filename, &st) != 0)
    {
        return false;
    }
    if (S_ISLNK(st.st_mode) == 1)
    {
        return false;
    }
    if (S_ISREG(st.st_mode) == 0)
    {
        return false;
    }
    return true;
}

off_t get_folder_size_recursive(GFile *folder)
{
    off_t total_size = 0;
    GError *error = NULL;
    GFileEnumerator *enumerator = g_file_enumerate_children(folder, G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &error);

    if (enumerator != NULL)
    {
        GFileInfo *info;
        while ((info = g_file_enumerator_next_file(enumerator, NULL, &error)) != NULL)
        {
            GFileType type = g_file_info_get_file_type(info);

            if (type == G_FILE_TYPE_REGULAR)
            {
                total_size += g_file_info_get_size(info);
            }
            else if (type == G_FILE_TYPE_DIRECTORY)
            {
                GFile *child = g_file_enumerator_get_child(enumerator, info);
                total_size += get_folder_size_recursive(child);
                g_object_unref(child);
            }
            g_object_unref(info);
        }
        g_object_unref(enumerator);
    }
    if (error != NULL)
    {
        g_error_free(error);
    }
    return total_size;
}
