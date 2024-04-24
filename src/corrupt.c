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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "corrupt.h"


struct _RaiderCorrupt
{
    GObject parent;
    RaiderFileRow* row;
    GFile* file;
    GTask* task;
    double progress;
    GCancellable* cancel;
};


G_DEFINE_TYPE(RaiderCorrupt, raider_corrupt, G_TYPE_OBJECT)

static GList* list_files(const char *directory, GList* list);
uint8_t corrupt_file(RaiderCorrupt* corrupt);
uint8_t corrupt_folder(RaiderCorrupt* corrupt);
uint8_t corrupt_unlink_file(const char *filename);
uint8_t corrupt_unlink_folder(const char *filename);
static uint8_t corrupt_step(RaiderCorrupt* corrupt, const off_t filesize, const char *pattern, int loop_num);


static void raider_corrupt_init(RaiderCorrupt *self)
{
    self->task = NULL;
    self->cancel = NULL;
}

static void raider_corrupt_class_init(RaiderCorruptClass *klass)
{
}

RaiderCorrupt *raider_corrupt_new(GFile* file, RaiderFileRow* row)
{
    RaiderCorrupt* corrupt = g_object_new(raider_corrupt_get_type(), NULL);
    corrupt->file = file;
    corrupt->row = row;
    return corrupt;
}

void shredding_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    RaiderCorrupt* corrupt = RAIDER_CORRUPT(source_object);

    GFileInfo *file_info = g_file_query_info(corrupt->file, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    if (g_file_info_get_file_type(file_info) == G_FILE_TYPE_DIRECTORY) {
        if (corrupt_folder(corrupt) == 0) {
            corrupt_unlink_folder(g_file_get_path(corrupt->file));
        }
        return;
    }

    if (corrupt_file(corrupt) == 0) {
        corrupt_unlink_file(g_file_get_path(corrupt->file));
    }
}

GCancellable* raider_corrupt_start_shredding(RaiderCorrupt* self, GAsyncReadyCallback func)
{
    self->cancel = g_cancellable_new();
    self->task = g_task_new(self, self->cancel, func, NULL);
    g_task_set_task_data (self->task, self->row, NULL);
    g_task_run_in_thread(self->task, shredding_thread);
    g_object_unref(self->task);

    return self->cancel;
}

static GList* list_files(const char *directory, GList* list) {
    const gchar *file_name;

    GDir *dir = g_dir_open(directory, 0, NULL);
    if (dir != NULL) {
        while ((file_name = g_dir_read_name(dir)) != NULL) {
            gchar *file_path = g_build_filename(directory, file_name, NULL);
            // If it is another directory, recurse.
            if (g_file_test(file_path, G_FILE_TEST_IS_DIR)) {
                // Make sure it is not the current or parent directories.
                if (strcmp(file_name, ".") != 0 && strcmp(file_name, "..") != 0) {
                    list_files(file_path, list);
                }
            } else {
                list = g_list_prepend(list, file_path);
            }
        }
        g_dir_close(dir);
    }
    return list;
}

uint8_t corrupt_folder(RaiderCorrupt* corrupt)
{
    fprintf(stderr, "corrupt: folder shredding unimplemented\n");

    return 0;
}

uint8_t corrupt_file(RaiderCorrupt* corrupt)
{
    // The length of these strings MUST be 3 characters each. Otherwise it will behave in a undefined manner.
    const char* steps[] = {"\x77\x77\x77", "\x76\x76\x76",
         "\x33\x33\x33", "\x35\x35\x35",
         "\x55\x55\x55", "\xAA\xAA\xAA",
         "\x44\x44\x44", "\x55\x55\x55",
         "\x66\x66\x66", "\x77\x77\x77"};

    int steps_num = sizeof(steps) / sizeof(steps[0]);
    uint8_t ret = 0;
    char* filename = g_file_get_path(corrupt->file);
    struct stat st;

    // Run some checks on the file.
    if(stat(filename, &st) != 0)
    {
        fprintf(stderr, "corrupt: current file not found\n");
        ret = 1;
    }
    if (S_ISREG(st.st_mode) == 0)
    {
        fprintf(stderr, "corrupt: current file is not a regular file\n");
        ret = 1;
    }

    // Set progress at zero.
    corrupt->progress = 0.0;
    raider_file_row_set_progress_num(corrupt->row, corrupt->progress);
    g_main_context_invoke (NULL, raider_file_row_set_progress, corrupt->row);

    // Shred the file.
    off_t filesize = st.st_size;
    uint8_t i;
    for (i = 0; i < steps_num; i++)
    {
        if (corrupt_step(corrupt, filesize, steps[i], i) != 0)
        {
            ret = 1;
            break;
        }

        double current = ((double)(i+1)/(double)steps_num);
        corrupt->progress = current;
        raider_file_row_set_progress_num(corrupt->row, corrupt->progress);
        g_main_context_invoke (NULL, raider_file_row_set_progress, corrupt->row);

    }

    return ret;
}

static uint8_t corrupt_step(RaiderCorrupt* corrupt, const off_t filesize, const char *pattern, int loop_num)
{
    char* filename = g_file_get_path(corrupt->file);
    int ret = 0;
    int length = 3; // The length of each element in the steps array below.

    // Run some checks.
    FILE* fp = fopen(filename,  "r+");
    if (fp == NULL)
    {
        ret = 1;
        return ret;
    }

    off_t i;
    off_t times = (filesize / length) + (filesize % length);
    for (i = 0; i < times; i++)
    {
        fwrite(pattern, sizeof(char), length, fp);
        if (g_task_return_error_if_cancelled (corrupt->task)) {fclose(fp);return 1;}
    }

    fclose(fp);
    return ret;
}

/* Removes folders also */
uint8_t corrupt_unlink_file(const char *filename)
{
    uint8_t ret = 0;

    if (remove(filename) != 0)
    {
        ret = 1;
    }

    return ret;
}

uint8_t corrupt_unlink_folder(const char *filename)
{
    fprintf(stderr, "corrupt: folder unlink unimplemented\n");
    return 0;
}
