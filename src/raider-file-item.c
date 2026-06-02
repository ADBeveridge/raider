#include "raider-file-item.h"

struct _RaiderFileItem {
    GObject parent_instance;
    GFile *file;
    gchar *name;
    gchar *path;
    gboolean is_folder;
    GMutex progress_mutex;
    GValue progress_gvalue;

    GCancellable *cancel;
};

G_DEFINE_TYPE(RaiderFileItem, raider_file_item, G_TYPE_OBJECT)

static void raider_file_item_init(RaiderFileItem *self)
{
    g_mutex_init(&self->progress_mutex);

    GValue progress = G_VALUE_INIT;
    self->progress_gvalue = progress;
    g_value_init(&self->progress_gvalue, G_TYPE_DOUBLE);
    g_value_set_double(&self->progress_gvalue, 0.0);

    self->cancel = NULL;
}

static void raider_file_item_dispose(GObject *object)
{
    RaiderFileItem *self = RAIDER_FILE_ITEM(object);
    g_clear_object(&self->file);
    g_clear_pointer(&self->name, g_free);
    g_clear_pointer(&self->path, g_free);

    // Clean up the thread locks and values
    g_mutex_clear(&self->progress_mutex);
    g_value_unset(&self->progress_gvalue);

    G_OBJECT_CLASS(raider_file_item_parent_class)->dispose(object);
}

static void raider_file_item_class_init(RaiderFileItemClass *klass)
{
    G_OBJECT_CLASS(klass)->dispose = raider_file_item_dispose;

    g_signal_new("progress-changed", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

RaiderFileItem *raider_file_item_new(GFile *file)
{
    RaiderFileItem *self = g_object_new(RAIDER_TYPE_FILE_ITEM, NULL);

    self->file = g_object_ref(file);
    self->name = g_file_get_basename(file);
    self->path = g_file_get_path(file);

    return self;
}

// This function does not operate in the the main context, unlike the set_progress function.
void raider_file_row_set_progress_value(RaiderFileItem* item, double progress)
{
    if(g_mutex_trylock (&item->progress_mutex) == FALSE)
        return;

    g_value_set_double(&item->progress_gvalue, progress);

    g_mutex_unlock(&item->progress_mutex);
}

double raider_file_item_get_progress(RaiderFileItem *item)
{
    // Safely lock, read, and unlock
    g_mutex_lock(&item->progress_mutex);
    double current_progress = g_value_get_double(&item->progress_gvalue);
    g_mutex_unlock(&item->progress_mutex);

    return current_progress;
}

GFile *raider_file_item_get_file(RaiderFileItem *item)
{
    return item->file;
}

gchar *raider_file_item_get_name(RaiderFileItem *item)
{
    return item->name;
}

gchar *raider_file_item_get_path(RaiderFileItem *item)
{
    return item->path;
}

gboolean raider_file_item_is_folder(RaiderFileItem *item)
{
    return item->is_folder;
}
