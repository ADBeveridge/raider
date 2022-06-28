#include <gtk/gtk.h>
#include <adwaita.h>

#include "raider-rd-popover.h"

struct _RaiderRdPopover {
	GtkPopover parent;

    /* Device selector data items. */
    GVolumeMonitor* monitor;
    GList* mount_list;

    GtkRevealer* nal_revealer;
    GtkRevealer* dl_revealer;
    GtkListBox* drive_list_box;
};

G_DEFINE_TYPE(RaiderRdPopover, raider_rd_popover, GTK_TYPE_POPOVER)

void on_mount_added(GtkWidget *widget, gpointer data)
{
    RaiderRdPopover* popover = RAIDER_RD_POPOVER(data);

    gtk_revealer_set_reveal_child(popover->dl_revealer, TRUE);
    gtk_revealer_set_reveal_child(popover->nal_revealer, FALSE);
}

static void raider_rd_popover_class_init(RaiderRdPopoverClass *class)
{
	gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/com/github/ADBeveridge/Raider/raider-rd-popover.ui");
}


static void raider_rd_popover_init(RaiderRdPopover *self)
{
	gtk_widget_init_template(GTK_WIDGET(self));

    /* Create monitor of mounted drives. */
    self->monitor = g_volume_monitor_get();
    self->mount_list = g_volume_monitor_get_mounts (self->monitor);
    g_signal_connect(self->monitor, "mount-added", G_CALLBACK(on_mount_added), self);

    /* Initial view. May be changed immediately. */
    gtk_revealer_set_reveal_child(self->dl_revealer, FALSE);
    gtk_revealer_set_reveal_child(self->nal_revealer, TRUE);

    /* List drives on startup. */
    GList *l;
    for (l = self->mount_list; l != NULL; l = l->next)
    {
        gchar* name = g_mount_get_name(l->data);
        g_free(name);

        gtk_revealer_set_reveal_child(self->dl_revealer, TRUE);
        gtk_revealer_set_reveal_child(self->nal_revealer, FALSE);
    }
}

