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

uint8_t corrupt_file(RaiderCorrupt* corrupt);
uint8_t corrupt_unlink_file(const char *filename);


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

    if (corrupt_file(corrupt) == 0)
        corrupt_unlink_file(g_file_get_path(corrupt->file));
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


static uint8_t corrupt_step(RaiderCorrupt* corrupt, const off_t filesize, const char *pattern, int loop_num)
{
    char* filename = g_file_get_path(corrupt->file);
    int ret = 0;

    // Run some checks.
    FILE* fp = fopen(filename,  "r+");
    if (fp == NULL)
    {
        ret = 1;
        return ret;
    }
    uint8_t length = (uint8_t) strlen(pattern);
    if (length <= 0)
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

        double current = ((double)loop_num/32.0) + (double)i/times*1.0/32.0;

        // Only update the progress when the jump is large enough.
        if (current - corrupt->progress >= .01)
        {
            corrupt->progress = current;
            printf("%d, %f\n", loop_num, current);
            raider_file_row_set_progress_num(corrupt->row, corrupt->progress);
            g_main_context_invoke (NULL, raider_file_row_set_progress, corrupt->row);

        }
    }

    fclose(fp);
    return ret;
}

uint8_t corrupt_file(RaiderCorrupt* corrupt)
{
    const char* steps[] = {"\x77\x77\x77", "\x76\x76\x76",
         "\x33\x33\x33", "\x35\x35\x35",
         "\x55\x55\x55", "\xAA\xAA\xAA",
         "\x92\x49\x24", "\x49\x24\x92",
         "\x55\x55\x55", "\x20\x02\x03",
         "\x11\x11\x11", "\x01\x01\x01",
         "\x22\x22\x22", "\x33\x33\x33",
         "\x44\x44\x44", "\x55\x55\x55",
         "\x66\x66\x66", "\x77\x77\x77",
         "\x88\x88\x88", "\x99\x99\x99",
         "\xAA\xAA\xAA", "\xBB\xBB\xBB",
         "\xCC\xCC\xCC", "\xDD\xDD\xDD",
         "\xEE\xEE\xEE", "\xFF\xFF\xFF",
         "\x92\x49\x24", "\x49\x24\x92",
         "\x24\x92\x49", "\x6D\xB6\xDB",
         "\xB6\xDB\x6D", "\xDB\x6D\xB6"};

    int steps_num = sizeof(steps) / sizeof(steps[0]);
    uint8_t ret = 0;
    char* filename = g_file_get_path(corrupt->file);
    struct stat st;

    // Run some checks on the file.
    if(stat(filename, &st) != 0)
    {
        fprintf(stderr, "corrupt: '%s' not found\n", filename);
        ret = 1;
    }
    if (S_ISREG(st.st_mode) == 0)
    {
        fprintf(stderr, "corrupt: '%s' is not a regular file\n", filename);
        ret = 1;
    }

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
    }

    return ret;
}

uint8_t corrupt_unlink_file(const char *filename)
{
    uint8_t ret = 0;

    if (remove(filename) != 0)
    {
        ret = 1;
    }

    return ret;
}

