#include <gtk/gtk.h>
#include "raider-progress-info-popover.h"

struct _RaiderProgressInfoPopover
{
    GtkPopover parent;
};

G_DEFINE_TYPE (RaiderProgressInfoPopover, raider_progress_info_popover, GTK_TYPE_POPOVER)

static void
raider_progress_info_popover_init (RaiderProgressInfoPopover *row)
{
	gtk_widget_init_template (GTK_WIDGET (row));
}

static void
raider_progress_info_popover_dispose (GObject *obj)
{
    G_OBJECT_CLASS (raider_progress_info_popover_parent_class)->dispose (obj);
}

static void
raider_progress_info_popover_class_init (RaiderProgressInfoPopoverClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = raider_progress_info_popover_dispose;

    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/raider/ui/raider-progress-info-popover.ui");
    //gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), RaiderProgressInfoPopover, button_box);
}

RaiderProgressInfoPopover *raider_progress_info_popover_new (const char *str)
{
    RaiderProgressInfoPopover *popover = g_object_new (raider_progress_info_popover_get_type (), NULL);
    return popover;
}

