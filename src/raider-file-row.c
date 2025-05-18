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

#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib/gi18n.h>
#include "raider-window.h"
#include "raider-file-row.h"
#include "raider-progress-paintable.h"
#include "raider-progress-info-popover.h"
#include "corrupt.h"

struct _RaiderFileRow
{
    AdwActionRow parent;

    /* Graphical controls. */
    GtkButton *progress_button;
    GtkButton *remove_button;

    /* Progress widgets. */
    RaiderProgressInfoPopover *popover;
    GdkPaintable *progress_paintable;
    GtkWidget *progress_paintable_image;
    GtkRevealer *progress_revealer;
    GtkRevealer *remove_revealer;

    /* Data items. */
    GFile *file;

    /* Variables to keep tabs on shredding. */
    GCancellable* cancel;
    GMutex mutex; // This is locked upon shredding, and only unlocked when the shredding is done (cancelled or end of file)
    GValue progress_gvalue;
    GMutex progress_mutex;
};

G_DEFINE_TYPE(RaiderFileRow, raider_file_row, ADW_TYPE_ACTION_ROW)

void raider_file_row_close (GtkWidget* window, gpointer data);

static void raider_file_row_dispose(GObject *obj)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(obj);

    g_object_unref(row->file);


    G_OBJECT_CLASS(raider_file_row_parent_class)->dispose(obj);
}

static void raider_file_row_init(RaiderFileRow *row)
{
    gtk_widget_init_template(GTK_WIDGET(row));

    row->popover = raider_progress_info_popover_new();
    gtk_widget_set_parent(GTK_WIDGET(row->popover), GTK_WIDGET(row->progress_button));
    g_signal_connect_swapped(row->progress_button, "clicked", G_CALLBACK(gtk_popover_popup), row->popover);
    g_signal_connect(row->remove_button, "clicked", G_CALLBACK(raider_file_row_close), row);
    g_mutex_init(&row->mutex);

    row->progress_paintable = raider_progress_paintable_new (GTK_WIDGET(row->progress_button));
    row->progress_paintable_image = gtk_image_new_from_paintable(row->progress_paintable);
    gtk_button_set_child(row->progress_button, row->progress_paintable_image);

    GValue progress = G_VALUE_INIT;
    row->progress_gvalue = progress;
    g_value_init(&row->progress_gvalue, G_TYPE_DOUBLE);
    g_value_set_double(&row->progress_gvalue, 0.0);

    row->cancel = NULL;
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

gchar *raider_file_row_get_filename(RaiderFileRow *row)
{
    return g_file_get_path(row->file);
}

// Remove file row.
void raider_file_row_close (GtkWidget* widget, gpointer data)
{
    RaiderFileRow* row = RAIDER_FILE_ROW(data);

    // If any one file was completed, show the finish notification.
    if (row->cancel != NULL && !g_cancellable_is_cancelled(row->cancel))
    {
        gpointer window = gtk_widget_get_root(GTK_WIDGET(data));
        raider_window_set_show_notification(window, TRUE);
    }

    raider_window_close_file(data, GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(data))));
    GtkListBox *list_box = GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(data)));
    gtk_list_box_remove(list_box, GTK_WIDGET(data));
}

/** Shredding section */
void shredding_finished(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    RaiderFileRow* row = g_task_get_task_data(G_TASK(res));

    g_mutex_unlock (&row->mutex);

    // Reset cancellable object.
    g_object_unref(row->cancel);
    row->cancel = NULL;

    /* Make sure that the user can use the window after the row is destroyed. */
    gtk_widget_set_visible(GTK_WIDGET(row->popover), FALSE);

    // If the shredding completed error free, then remove this file row.
    if (g_task_had_error (G_TASK (res)) == FALSE)
        raider_file_row_close(NULL, row);
}

void raider_file_row_launch_shredding(gpointer data)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(data);

    // Only unlocked when corrupt code is no longer running.  Used to make
    // sure that the UX is only updated when no file operations are running.
    g_mutex_lock (&row->mutex);

    gpointer window = gtk_widget_get_root(GTK_WIDGET(row));
    raider_window_set_show_notification(window, TRUE);

    /* Change the button. */
    adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), GTK_WIDGET(row->progress_button));
    gtk_revealer_set_reveal_child(row->remove_revealer, FALSE);
    gtk_revealer_set_reveal_child(row->progress_revealer, TRUE);

    RaiderCorrupt* corrupt = raider_corrupt_new(row->file, row);
    row->cancel = raider_corrupt_start_shredding (corrupt, shredding_finished);
}
/* End of shredding section. */

/* This is called when the user clicks abort while shredding. */
void raider_file_row_shredding_abort(gpointer data)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(data);

    /* Change the row view. */
    adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), NULL);
    gtk_revealer_set_reveal_child(row->remove_revealer, TRUE);
    gtk_revealer_set_reveal_child(row->progress_revealer, FALSE);

    // Mark file row as aborted, and corrupt code will stop running when it polls the cancellable.
    g_cancellable_cancel(row->cancel);

    // This will block until the function has returned.
    g_mutex_lock (&row->mutex);
    g_mutex_unlock (&row->mutex);
}

// This function does not operate in the the main context, unlike the set_progress function.
void raider_file_row_set_progress_value(RaiderFileRow* row, double progress)
{

    if(g_mutex_trylock (&row->progress_mutex) == FALSE)
        return;

    g_value_set_double(&row->progress_gvalue, progress);

    g_mutex_unlock(&row->progress_mutex);
}

// Called from corrupt thread using g_main_context_invoke().
gboolean raider_file_row_update_progress_ui(gpointer data)
{
    RaiderFileRow* row = RAIDER_FILE_ROW(data);

    // Set paintable progress.
    g_object_set_property(G_OBJECT(row->progress_paintable), "progress", &row->progress_gvalue);

    // Set popover progress.
    raider_progress_info_popover_set_progress (row->popover, g_value_get_double(&row->progress_gvalue));

    return FALSE;
}

RaiderFileRow *raider_file_row_new(GFile *file)
{
    RaiderFileRow *row = g_object_new(RAIDER_TYPE_FILE_ROW, NULL);

    row->file = file;

    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), g_file_get_basename(row->file));
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), g_file_get_path(row->file));

    return row;
}
