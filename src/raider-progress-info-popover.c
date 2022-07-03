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
#include "raider-progress-info-popover.h"

struct _RaiderProgressInfoPopover
{
    GtkPopover parent;
    GtkWidget *progress_bar;
};

G_DEFINE_TYPE(RaiderProgressInfoPopover, raider_progress_info_popover, GTK_TYPE_POPOVER)

static void
raider_progress_info_popover_init(RaiderProgressInfoPopover *row)
{
    gtk_widget_init_template(GTK_WIDGET(row));
}

void raider_progress_info_popover_set_progress(RaiderProgressInfoPopover *popover, gdouble fraction)
{
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(popover->progress_bar), fraction);
}

void raider_progress_info_popover_set_text(RaiderProgressInfoPopover *popover, gchar *text)
{
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(popover->progress_bar), text);
}

static void
raider_progress_info_popover_class_init(RaiderProgressInfoPopoverClass *klass)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/Raider/raider-progress-info-popover.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderProgressInfoPopover, progress_bar);
}

RaiderProgressInfoPopover *raider_progress_info_popover_new(GtkWidget *relative)
{
    return g_object_new(raider_progress_info_popover_get_type(), NULL);
}
