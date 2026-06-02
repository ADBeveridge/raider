#ifndef CORRUPT_H
#define CORRUPT_H

#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>

#define CORRUPT_TYPE (corrupt_get_type())

G_DECLARE_FINAL_TYPE(Corrupt, corrupt, CORRUPT, CORRUPT, GObject)

Corrupt *corrupt_new(void);
void corrupt_start_shredding_async(Corrupt *self, GList *files_to_shred, GCancellable *cancel, GAsyncReadyCallback callback, gpointer user_data);
gboolean corrupt_start_shredding_finish(Corrupt *self, GAsyncResult *res, GError **error);

#endif // CORRUPT_H
