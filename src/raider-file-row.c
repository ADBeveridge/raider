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
#include "raider-progress-icon.h"
#include "raider-progress-info-popover.h"
#include "corrupt.h"

struct _RaiderFileRow
{
    AdwActionRow parent;

    /* Graphical controls. */
    GtkButton *progress_button;
    GtkButton *remove_button;

    GtkRevealer *progress_revealer;
    GtkRevealer *remove_revealer;

    /* Progress widgets. */
    GtkBox *progress_widgets_box;
    RaiderProgressInfoPopover *popover;

    RaiderProgressIcon *icon;
    GtkWidget *spinner;

    /* Notification widget. */
    GNotification *notification;
    gchar *notification_title;
    gchar *notification_subtitle;

    /* Data items. */
    GFile *file;

    GCancellable* cancel;
    GMutex mutex;
    GCond cond;

    gboolean aborted; // Aborted can mean anything to prevent the file from shredding.
};

G_DEFINE_TYPE(RaiderFileRow, raider_file_row, ADW_TYPE_ACTION_ROW)

static void raider_file_row_close (GtkWidget* window, gpointer data);
static void raider_popup_popover(GtkWidget *widget, gpointer data);

static void raider_file_row_dispose(GObject *obj)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(obj);

    g_object_unref(row->file);

    gtk_widget_unparent(GTK_WIDGET(row->popover));

    g_free(row->notification_title);
    row->notification_title = NULL;
    g_free(row->notification_subtitle);
    row->notification_subtitle = NULL;

    G_OBJECT_CLASS(raider_file_row_parent_class)->dispose(obj);
}

static void raider_file_row_init(RaiderFileRow *row)
{
    gtk_widget_init_template(GTK_WIDGET(row));

    row->popover = raider_progress_info_popover_new();
    gtk_widget_set_parent(GTK_WIDGET(row->popover), GTK_WIDGET(row->progress_button));
    g_signal_connect(row->progress_button, "clicked", G_CALLBACK(raider_popup_popover), row->popover);

    /* Setup remove row. */
    g_signal_connect(row->remove_button, "clicked", G_CALLBACK(raider_file_row_close), row);

    row->aborted = FALSE; // We have not been aborted.
    row->cancel = NULL;
}

static void raider_file_row_class_init(RaiderFileRowClass *klass)
{
    G_OBJECT_CLASS(klass)->dispose = raider_file_row_dispose; /* Override. */

    /* Load the progress icon into the type system. */
    g_type_ensure(RAIDER_TYPE_PROGRESS_ICON);

    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/Raider/raider-file-row.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, remove_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, remove_revealer);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_revealer);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, progress_widgets_box);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, spinner);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(klass), RaiderFileRow, icon);
}

gchar *raider_file_row_get_filename(RaiderFileRow *row)
{
    return g_file_get_path(row->file);
}
/* This version of delete tells the raider_window_close_file function to show a toast that shredding is done. */
static void raider_file_row_delete(GtkWidget *widget, gpointer data)
{
    RaiderFileRow* row = RAIDER_FILE_ROW(data);
    if (row->aborted == TRUE)
    {
        // One file being aborted is enough not to show the notification.
        gpointer window = gtk_widget_get_root(GTK_WIDGET(data));
        raider_window_set_show_notification(window, FALSE);
    }

    raider_window_close_file(data, GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(data))));
    GtkListBox *list_box = GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(data)));
    gtk_list_box_remove(list_box, GTK_WIDGET(data));
}

/* This is shown when the user clicks on the progress icon. The popover shows more information. */
static void raider_popup_popover(GtkWidget *widget, gpointer data)
{
    gtk_popover_popup(GTK_POPOVER(data));
}

static void raider_file_row_close (GtkWidget* window, gpointer data)
{
    RaiderFileRow* row = RAIDER_FILE_ROW(data);
    row->aborted = TRUE; // We are not shredding the file.
    raider_file_row_delete(NULL, data);
}

/** Shredding section */
static void shredding_finished(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    RaiderFileRow* row = RAIDER_FILE_ROW(source_object);

    /* Make sure that the user can use the window after the row is destroyed. */
    gtk_widget_set_visible(GTK_WIDGET(row->popover), FALSE);

    // If the shredding completed error free, then remove this file row.
    if (g_task_had_error (G_TASK (res)) == FALSE)
        raider_file_row_delete(NULL, row);

    // All done!.
    g_mutex_unlock (&row->mutex);
}
static void shredding_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    RaiderFileRow* row = RAIDER_FILE_ROW(source_object);

    struct _corrupt_data *corrupt_data = malloc(sizeof *corrupt_data);
    corrupt_data->task = task;
    corrupt_data->popover = row->popover;
    corrupt_data->icon = row->icon;

    if (corrupt_file(g_file_get_path(row->file), corrupt_data) == 0)
        corrupt_unlink_file(g_file_get_path(row->file));

    free(corrupt_data);
}
void raider_file_row_launch_shredding(gpointer data)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(data);

    row->aborted = FALSE;
    g_mutex_lock (&row->mutex); // This is used by the abort function.

    gpointer window = gtk_widget_get_root(GTK_WIDGET(row));
    raider_window_set_show_notification(window, TRUE);

    /* Change the button. */
    gtk_revealer_set_reveal_child(row->remove_revealer, FALSE);
    gtk_revealer_set_reveal_child(row->progress_revealer, TRUE);

    gtk_widget_set_visible(row->spinner, FALSE);
    gtk_widget_set_visible(GTK_WIDGET(row->icon), TRUE);

    /* Set the activatable widget. */
    adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), GTK_WIDGET(row->progress_button));

    if (row->cancel) g_object_unref(row->cancel);
    row->cancel = g_cancellable_new();

    GTask *task = g_task_new(row, row->cancel, shredding_finished, NULL);
    g_task_run_in_thread(task, shredding_thread);
    g_object_unref(task);
}
/* End of shredding section. */

/* This is called when the user clicks abort. */
void raider_file_row_shredding_abort(gpointer data)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(data);

    /* Change the row view. */
    adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), NULL);
    gtk_revealer_set_reveal_child(row->remove_revealer, TRUE);
    gtk_revealer_set_reveal_child(row->progress_revealer, FALSE);
    gtk_spinner_stop(GTK_SPINNER(row->spinner));

    g_cancellable_cancel(row->cancel);
    row->aborted = TRUE;

    // This will block until the function has returned.
    g_mutex_lock (&row->mutex);
    g_mutex_unlock (&row->mutex);
}

void raider_file_row_set_progress(gpointer data)
{
    struct _corrupt_data *corrupt_data = data;
    raider_progress_icon_set_progress(corrupt_data->icon, corrupt_data->progress);
    raider_progress_info_popover_set_progress (corrupt_data->popover, corrupt_data->progress);
}

RaiderFileRow *raider_file_row_new(GFile *file)
{
    RaiderFileRow *row = g_object_new(RAIDER_TYPE_FILE_ROW, NULL);

    row->file = file;

    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), g_file_get_basename(row->file));
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), g_file_get_path(row->file));

    /* Notification stuff. */
    row->notification_title = g_strconcat(_("Shredded "), g_file_get_basename(row->file), NULL);
    row->notification = g_notification_new(row->notification_title);
    row->notification_subtitle = NULL;

    return row;
}
