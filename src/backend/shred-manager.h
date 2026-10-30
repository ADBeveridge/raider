#ifndef SHRED_MANAGER_H
#define SHRED_MANAGER_H

#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>
#include "raider-file-item.h"

#define SHRED_MANAGER_TYPE (shred_manager_get_type())

G_DECLARE_FINAL_TYPE(ShredManager, shred_manager, SHRED_MANAGER, MANAGER, GObject)

ShredManager *shred_manager_new(void);
GListModel *shred_manager_get_model(ShredManager *self);
void shred_manager_add_file(ShredManager *self, GFile *file);
void shred_manager_clear_files(ShredManager *self);
void shred_manager_remove_file(ShredManager *self, RaiderFileItem *item);
void shred_manager_task_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
gboolean shred_manager_start_shredding_finish(ShredManager *self, GAsyncResult *res, GError **error);

#endif // SHRED_MANAGER_H
