#include "raider-file-item.h"

struct _RaiderFileItem
{
    GObject parent_instance;
    GCancellable *cancel;
    GFile *file;
    gchar *name;
    gchar *path;
    gboolean is_folder;

    double progress;
};

G_DEFINE_TYPE(RaiderFileItem, raider_file_item, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_PROGRESS,
    N_PROPS
};

static GParamSpec *obj_properties[N_PROPS] = {
    NULL,
};

static void raider_file_item_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    RaiderFileItem *self = RAIDER_FILE_ITEM(object);
    if (property_id == PROP_PROGRESS)
        g_value_set_double(value, self->progress);
    else
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
}

static void raider_file_item_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    RaiderFileItem *self = RAIDER_FILE_ITEM(object);
    if (property_id == PROP_PROGRESS)
        self->progress = g_value_get_double(value);
    else
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
}

static void raider_file_item_dispose(GObject *object)
{
    RaiderFileItem *self = RAIDER_FILE_ITEM(object);
    g_clear_object(&self->file);
    g_clear_pointer(&self->name, g_free);
    g_clear_pointer(&self->path, g_free);

    G_OBJECT_CLASS(raider_file_item_parent_class)->dispose(object);
}

static void raider_file_item_class_init(RaiderFileItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = raider_file_item_dispose;
    object_class->get_property = raider_file_item_get_property;
    object_class->set_property = raider_file_item_set_property;

    obj_properties[PROP_PROGRESS] = g_param_spec_double("progress", "Progress", "Shredding progress", 0.0, 1.0, 0.0, G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

    g_object_class_install_properties(object_class, N_PROPS, obj_properties);
}

static void raider_file_item_init(RaiderFileItem *self)
{
    self->progress = 0.0;
}

RaiderFileItem *raider_file_item_new(GFile *file)
{
    RaiderFileItem *self = g_object_new(RAIDER_TYPE_FILE_ITEM, NULL);

    self->file = g_object_ref(file);
    self->name = g_file_get_basename(file);
    self->path = g_file_get_path(file);

    return self;
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

// Progress structure.
typedef struct
{
    RaiderFileItem *item;
    double progress;
} ProgressPayload;

static gboolean dispatch_progress_to_main_thread(gpointer data)
{
    ProgressPayload *payload = data;

    payload->item->progress = payload->progress;
    g_object_notify_by_pspec(G_OBJECT(payload->item), obj_properties[PROP_PROGRESS]);

    g_object_unref(payload->item);
    g_free(payload);

    return G_SOURCE_REMOVE;
}

void raider_file_item_set_progress_safe(RaiderFileItem *self, double progress)
{
    ProgressPayload *payload = g_new(ProgressPayload, 1);

    payload->item = g_object_ref(self);
    payload->progress = progress;

    g_main_context_invoke(NULL, dispatch_progress_to_main_thread, payload);
}
