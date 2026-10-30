#include "bucket.h"
#include "job.h"
#include "raider-file-item.h"
#include "shred-manager.h"
#include "utility.h"

void bucket_add_file(Bucket *self, RaiderFileItem *item)
{
    self->files = g_list_append(self->files, item);
}

void bucket_shred(Bucket *self)
{
    for (GList *l = self->files; l != NULL; l = l->next)
    {
        if (self->cancel && g_cancellable_is_cancelled(self->cancel))
        {
            break;
        }

        RaiderFileItem *item = RAIDER_FILE_ITEM(l->data);

        raider_file_item_set_started_async(item);

        bool res;
        if (raider_file_item_is_folder(item))
        {
            res = corrupt_folder(item, &self->strategy, self->cancel);
        }
        else
        {
            res = corrupt_file(item, &self->strategy, self->cancel);
        }

        if (!res)
        {
            g_printerr("File shredding failed!\n");
            continue;
        }

        raider_file_item_set_finished_async(item);
    }
}
