#include <glib.h>
#include <nautilus-extension.h>
#include <string.h>
#include <time.h>

static GType provider_types[1];
static GType raider_extension_type;
static GObjectClass *parent_class;

typedef struct {
	GObject parent_slot;
} RaiderExtension;

typedef struct {
	GObjectClass parent_slot;
} RaiderExtensionClass;

/* nautilus extension interface */
void nautilus_module_initialize (GTypeModule  *module);
void nautilus_module_shutdown (void);
void nautilus_module_list_types (const GType **types, int *num_types);
GType raider_extension_get_type (void);

static void raider_extension_register_type (GTypeModule *module);

/* menu filler */
static GList * raider_extension_get_file_items (NautilusMenuProvider *provider, GtkWidget *window, GList *files);

#if 0
static GList * raider_extension_get_background_items (NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder);
static GList * raider_extension_get_toolbar_items (NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder);
#endif

/* command callback */
static void do_stuff_cb (NautilusMenuItem *item, gpointer user_data);

void nautilus_module_initialize (GTypeModule  *module)
{
        raider_extension_register_type (module);

        provider_types[0] = raider_extension_get_type ();
}

void nautilus_module_shutdown (void) {}

void nautilus_module_list_types (const GType **types, int *num_types)
{
        *types = provider_types;
        *num_types = G_N_ELEMENTS (provider_types);
}

GType raider_extension_get_type (void)
{
        return raider_extension_type;
}

static void raider_extension_instance_init (RaiderExtension *object)
{
}

static void raider_extension_class_init(RaiderExtensionClass *class)
{
	parent_class = g_type_class_peek_parent (class);
}

static void raider_extension_menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
	iface->get_file_items = raider_extension_get_file_items;
}

static void raider_extension_register_type (GTypeModule *module)
{
        static const GTypeInfo info = {
                sizeof (RaiderExtensionClass),
                (GBaseInitFunc) NULL,
                (GBaseFinalizeFunc) NULL,
                (GClassInitFunc) raider_extension_class_init,
                NULL,
                NULL,
                sizeof (RaiderExtension),
                0,
                (GInstanceInitFunc) raider_extension_instance_init,
        };

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) raider_extension_menu_provider_iface_init,
		NULL,
		NULL
	};

        raider_extension_type = g_type_module_register_type (module, G_TYPE_OBJECT, "RaiderExtension", &info, 0);

	g_type_module_add_interface (module, raider_extension_type, NAUTILUS_TYPE_MENU_PROVIDER, &menu_provider_iface_info);
}


static GList * raider_extension_get_file_items (NautilusMenuProvider *provider, GtkWidget *window, GList *files)
{
        NautilusMenuItem *item;
        GList *ret;

        item = nautilus_menu_item_new ("RaiderExtension::shred", "Shred", "Shred the selected files", "com.github.ADBeveridge.Raider");
        g_signal_connect (item, "activate", G_CALLBACK (do_stuff_cb), provider);
        g_object_set_data_full ((GObject*) item, "raider_extension_files", nautilus_file_info_list_copy (files), (GDestroyNotify)nautilus_file_info_list_free);
        ret = g_list_append (NULL, item);

        return ret;
}

static void do_stuff_cb (NautilusMenuItem *item, gpointer user_data)
{
        GList *files;
        GList *l;

	char *final_command = strdup("raider ");
	
        files = g_object_get_data ((GObject *) item, "raider_extension_files");

        for (l = files; l != NULL; l = l->next) {
        	
                NautilusFileInfo *file = NAUTILUS_FILE_INFO (l->data);
                
                char *name = nautilus_file_info_get_uri (file);
          
                
                /* Duplicate original, then free. */
                gchar *dupli = strdup (final_command);
                g_free (final_command);
                final_command = NULL;
                
                final_command = g_strconcat(dupli, " ", name, NULL);
                
                g_free (dupli);
                g_free (name);
        }
        g_spawn_command_line_async (final_command, NULL);
}
