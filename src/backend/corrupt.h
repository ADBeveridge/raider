#ifndef CORRUPT_H
#define CORRUPT_H

#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>
#include "raider-file-item.h"

#define CORRUPT_TYPE (corrupt_get_type())

G_DECLARE_FINAL_TYPE(Corrupt, corrupt, CORRUPT, CORRUPT, GObject)

Corrupt *corrupt_new(void);
GListModel *corrupt_get_model(Corrupt *self);
void corrupt_add_file(Corrupt *self, GFile *file);
void corrupt_clear_files(Corrupt *self);
void corrupt_remove_file(Corrupt *self, RaiderFileItem *item);
void corrupt_start_shredding_async(Corrupt *self, GCancellable *cancel, GAsyncReadyCallback callback, gpointer user_data);
void shred_all_task_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
gboolean corrupt_start_shredding_finish(Corrupt *self, GAsyncResult *res, GError **error);

#endif // CORRUPT_H
