/* raider-file-row.c
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

#include "raider-file-row.h"
#include "backend/raider-file-item.h"
#include "raider-progress-info-popover.h"
#include "raider-progress-paintable.h"
#include "raider-window.h"
#include <adwaita.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

struct _RaiderFileRow
{
    AdwActionRow parent;

    GtkButton *progress_button;
    GtkButton *remove_button;
    GtkRevealer *progress_revealer;
    GtkRevealer *remove_revealer;

    RaiderProgressInfoPopover *popover;
    GdkPaintable *progress_paintable;
    GtkWidget *progress_paintable_image;

    RaiderFileItem *bound_item;
};

G_DEFINE_TYPE(RaiderFileRow, raider_file_row, ADW_TYPE_ACTION_ROW)

void raider_file_row_close(GtkWidget *window, gpointer data);

static void raider_file_row_dispose(GObject *obj)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(obj);

    gtk_widget_unparent(GTK_WIDGET(row->popover));

    // Clear image reference to paintable, and destroy our reference.
    gtk_image_set_from_paintable(GTK_IMAGE(row->progress_paintable_image), NULL);
    g_object_unref(row->progress_paintable);

    G_OBJECT_CLASS(raider_file_row_parent_class)->dispose(obj);
}

static void raider_file_row_init(RaiderFileRow *row)
{
    gtk_widget_init_template(GTK_WIDGET(row));

    row->popover = raider_progress_info_popover_new();
    gtk_widget_set_parent(GTK_WIDGET(row->popover), GTK_WIDGET(row->progress_button));
    g_signal_connect_swapped(row->progress_button, "clicked", G_CALLBACK(gtk_popover_popup), row->popover);
    g_signal_connect(row->remove_button, "clicked", G_CALLBACK(raider_file_row_close), row);

    row->progress_paintable = raider_progress_paintable_new(GTK_WIDGET(row->progress_button));
    row->progress_paintable_image = gtk_image_new_from_paintable(row->progress_paintable);
    gtk_button_set_child(row->progress_button, row->progress_paintable_image);
}

static void raider_file_row_class_init(RaiderFileRowClass *klass)
{
    G_OBJECT_CLASS(klass)->dispose = raider_file_row_dispose; /* Override. */

    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/Raider/raider-file-row.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, remove_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, remove_revealer);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_revealer);
}

RaiderFileRow *raider_file_row_new()
{
    RaiderFileRow *row = g_object_new(RAIDER_TYPE_FILE_ROW, NULL);
    return row;
}

void raider_file_row_bind_item(RaiderFileRow *self, RaiderFileItem *item)
{
    if (self->bound_item)
    {
        g_object_unref(self->bound_item);
    }
    self->bound_item = g_object_ref(item);

    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(self), raider_file_item_get_name(item));
    adw_action_row_set_subtitle(ADW_ACTION_ROW(self), raider_file_item_get_path(item));

    g_object_bind_property(item, "progress", self->progress_paintable, "progress", G_BINDING_SYNC_CREATE);
    g_object_bind_property(item, "progress", self->popover, "progress", G_BINDING_SYNC_CREATE);
}

// Remove file row.
void raider_file_row_close(GtkWidget *widget, gpointer data)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(data);

    // Find the main window
    RaiderWindow *window = RAIDER_WINDOW(gtk_widget_get_root(GTK_WIDGET(row)));

    if (row->bound_item != NULL)
    {
        raider_window_close_file(row->bound_item, window);
    }
}
