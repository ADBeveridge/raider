/* raider-window.c
 *
 * Copyright 2022 Alan Beveridge
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "raider-window.h"
#include "backend/corrupt.h"
#include "raider-config.h"
#include "backend/raider-file-item.h"
#include "raider-file-row.h"
#include <fcntl.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>

static gboolean on_drop(GtkDropTarget *target, const GValue *value, double x, double y, gpointer data);
static gboolean raider_window_check_file(GFile *file, gpointer data, gchar *title);
static void raider_window_start_shredding(GtkWidget *widget, gpointer data);
static void raider_window_abort_shredding(GtkWidget *widget, gpointer data);
static void raider_window_clear_files(GtkWidget *widget, gpointer data);
static GtkWidget *create_listbox_row(gpointer item, gpointer user_data);

struct _RaiderWindow
{
    AdwApplicationWindow parent_instance;

    GtkBox *contents_box;
    GtkStack *window_stack;
    GtkButton *open_button;
    GtkRevealer *open_revealer;
    GtkButton *clear_button;
    GtkButton *shred_button;
    GtkRevealer *shred_revealer;
    GtkButton *abort_button;
    GtkRevealer *abort_revealer;
    AdwToastOverlay *toast_overlay;
    GtkListBox *list_box;
    GtkDropTarget *target;

    Corrupt *corrupt;
    GCancellable *cancel_shredding;
    gboolean status; // Shredding or not.
    gboolean show_notification;
};

G_DEFINE_TYPE(RaiderWindow, raider_window, ADW_TYPE_APPLICATION_WINDOW)

static void raider_window_dispose(GObject *object)
{
    // RaiderWindow *self = RAIDER_WINDOW (object);
    G_OBJECT_CLASS(raider_window_parent_class)->dispose(object);
}

static void raider_window_class_init(RaiderWindowClass *klass)
{
    G_OBJECT_CLASS(klass)->dispose = raider_window_dispose;

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    gtk_widget_class_set_template_from_resource(widget_class, "/com/github/ADBeveridge/Raider/raider-window.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, open_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, open_revealer);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, clear_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, shred_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, abort_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, list_box);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, window_stack);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, shred_revealer);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, abort_revealer);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, toast_overlay);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, contents_box);
}

static void raider_window_init(RaiderWindow *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));

    g_signal_connect(self->clear_button, "clicked", G_CALLBACK(raider_window_clear_files), self);
    g_signal_connect(self->shred_button, "clicked", G_CALLBACK(raider_window_start_shredding), self);
    g_signal_connect(self->abort_button, "clicked", G_CALLBACK(raider_window_abort_shredding), self);
    g_signal_connect(self, "close-request", G_CALLBACK(raider_window_exit), NULL);

    // Setup drag and drop.
    self->target = gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_COPY);
    GType drop_types[] = {GDK_TYPE_FILE_LIST};
    gtk_drop_target_set_gtypes(self->target, drop_types, 1);
    g_signal_connect(self->target, "drop", G_CALLBACK(on_drop), self);
    gtk_widget_add_controller(GTK_WIDGET(self->contents_box), GTK_EVENT_CONTROLLER(self->target));

    // Setup backend.
    self->corrupt = corrupt_new();
    gtk_list_box_bind_model(self->list_box, corrupt_get_model(self->corrupt), create_listbox_row, self, NULL);

    self->cancel_shredding = g_cancellable_new();
    self->status = FALSE;
    self->show_notification = FALSE;
}

static GtkWidget *create_listbox_row(gpointer item, gpointer user_data)
{
    RaiderFileRow *row = raider_file_row_new();

    // You just push the data in immediately
    raider_file_row_bind_item(row, RAIDER_FILE_ITEM(item));

    return GTK_WIDGET(row);
}

static void raider_window_clear_files(GtkWidget *widget, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    corrupt_clear_files(window->corrupt);

    // Reset the UI state
    gtk_stack_set_visible_child_name(window->window_stack, "empty_page");
    gtk_revealer_set_reveal_child(window->shred_revealer, FALSE);
    gtk_revealer_set_reveal_child(window->abort_revealer, FALSE);
    gtk_revealer_set_reveal_child(window->open_revealer, TRUE);
}

static gboolean on_drop(GtkDropTarget *target, const GValue *value, double x, double y, gpointer data)
{
    /* GdkFileList is a boxed value so we use the boxed API. */
    GdkFileList *flist = g_value_get_boxed(value);

    /* Convert GSList to GList. */
    GSList *slist = gdk_file_list_get_files(flist);
    GSList *l;
    GList *file_list = NULL;
    for (l = slist; l != NULL; l = l->next)
    {
        file_list = g_list_append(file_list, g_file_dup(l->data));
    }
    g_slist_free(slist);

    raider_window_open_files(data, file_list);

    return TRUE;
}

static void raider_window_exit_response(GtkDialog *dialog, gchar *response, RaiderWindow *self)
{
    if (g_strcmp0(response, "exit") == 0)
    {
        // Because the first argument is NULL, the function will construe that to exit. This is a hack around the GTask callback system.
        raider_window_abort_shredding(NULL, GTK_WIDGET(self));
    }
}

gboolean raider_window_exit(RaiderWindow *win, gpointer data)
{
    if (win->status)
    {
        AdwDialog *dialog = adw_alert_dialog_new(_("Stop Shredding?"), _("Are you sure that you want to exit?"));
        g_signal_connect(dialog, "response", G_CALLBACK(raider_window_exit_response), win);

        adw_alert_dialog_add_responses(ADW_ALERT_DIALOG(dialog), "cancel", _("_Cancel"), "exit", _("_Exit"), NULL);
        adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog), "exit", ADW_RESPONSE_DESTRUCTIVE);
        adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "cancel");
        adw_alert_dialog_set_close_response(ADW_ALERT_DIALOG(dialog), "cancel");

        adw_dialog_present(dialog, GTK_WIDGET(win));
    }

    // Based on the value of this, the window will exit or will not.
    return win->status;
}

void raider_window_set_show_notification(RaiderWindow *window, gboolean show)
{
    window->show_notification = show;
}

void raider_window_show_toast(RaiderWindow *window, gchar *text)
{
    adw_toast_overlay_add_toast(window->toast_overlay, adw_toast_new(text));
}

/* This handles the application and window state. */
void raider_window_close_file(RaiderFileItem *target_item, RaiderWindow *window)
{
    corrupt_remove_file(window->corrupt, target_item);

    // Check the live count AFTER the removal
    if (g_list_model_get_n_items(corrupt_get_model(window->corrupt)) == 0)
    {
        gtk_stack_set_visible_child_name(window->window_stack, "empty_page");
        window->status = FALSE;

        if (window->show_notification == TRUE)
        {
            gchar *message = g_strdup(_("Finished shredding files"));

            gboolean active = gtk_window_is_active(GTK_WINDOW(window));
            if (!active)
            {
                GNotification *notification = g_notification_new(message);
                g_application_send_notification(G_APPLICATION(gtk_window_get_application(GTK_WINDOW(window))), NULL, notification);
            }
            else
                raider_window_show_toast(window, message);
            g_free(message);
        }

        /* Update the view. */
        gtk_revealer_set_reveal_child(window->shred_revealer, FALSE);
        gtk_revealer_set_reveal_child(window->abort_revealer, FALSE);
        gtk_revealer_set_reveal_child(window->open_revealer, TRUE);
        window->show_notification = TRUE;
    }
}

static void raider_window_open_files_finish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    RaiderWindow *window = RAIDER_WINDOW(source_object);

    GError *error = NULL;
    GList *valid_files = g_task_propagate_pointer(G_TASK(res), &error);
    if (error != NULL)
    {
        g_printerr("Failed to process files in background: %s\n", error->message);
        g_error_free(error);
        return;
    }

    for (GList *l = valid_files; l != NULL; l = l->next)
    {
        GFile *file = l->data;
        corrupt_add_file(window->corrupt, file);
    }

    if (g_list_model_get_n_items(corrupt_get_model(window->corrupt)) > 0)
    {
        gtk_stack_set_visible_child_name(GTK_STACK(window->window_stack), "list_page");
        gtk_revealer_set_reveal_child(GTK_REVEALER(window->shred_revealer), TRUE);
    }

    g_list_free(valid_files);
}

static void raider_window_open_files_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    GList *file_list = task_data;
    GList *valid_files = NULL;

    for (GList *l = file_list; l != NULL; l = l->next)
    {
        gboolean status = raider_window_check_file(l->data, NULL, NULL);
        if (status)
        {
            valid_files = g_list_append(valid_files, g_object_ref(l->data));
        }
    }
    g_list_free_full(file_list, g_object_unref);

    g_task_return_pointer(task, valid_files, (GDestroyNotify)g_list_free);
    // raider_window_open_file_finish() is called here.
}

void raider_window_open_files(RaiderWindow *window, GList *file_list)
{
    GList *cleaned_file_list = NULL;
    GListModel *model = corrupt_get_model(window->corrupt);
    guint n_items = g_list_model_get_n_items(model);

    // Check to make sure we haven't loaded the file yet.
    for (GList *l = file_list; l != NULL; l = l->next)
    {
        GFile *incoming_file = G_FILE(l->data);
        gboolean is_duplicate = FALSE;

        for (guint i = 0; i < n_items; i++)
        {
            RaiderFileItem *existing_item = g_list_model_get_item(model, i);
            GFile *existing_file = raider_file_item_get_file(existing_item);

            if (g_file_equal(incoming_file, existing_file))
            {
                is_duplicate = TRUE;
            }

            g_object_unref(existing_item);
            if (is_duplicate)
                break;
        }

        if (is_duplicate)
        {
            raider_window_show_toast(window, "File already loaded!");
            g_object_unref(incoming_file);
        }
        else
        {
            cleaned_file_list = g_list_append(cleaned_file_list, incoming_file);
        }
    }

    g_list_free(file_list);
    if (cleaned_file_list == NULL)
        return;

    GTask *task = g_task_new(window, NULL, raider_window_open_files_finish, NULL);
    g_task_set_task_data(task, cleaned_file_list, NULL);
    g_task_run_in_thread(task, raider_window_open_files_thread);
    g_object_unref(task);
}

/* Check the file to make sure we can corrupt it. */
static gboolean raider_window_check_file(GFile *file, gpointer data, gchar *title)
{
    gchar *filename = g_file_get_path(file);

    if (g_file_query_exists(file, NULL) == FALSE)
    {
        g_free(filename);
        return FALSE;
    }

    /* Test if we can write. */
    if (g_access(filename, W_OK) != 0)
    {
        g_free(filename);
        return FALSE;
    }

    g_free(filename);
    return TRUE;
}

static void raider_window_shred_files_finish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    RaiderWindow *window = RAIDER_WINDOW(user_data);
    Corrupt *corrupt = (Corrupt *)source_object;
    GError *error = NULL;

    /* Update the view. */
    gtk_revealer_set_reveal_child(window->open_revealer, TRUE);
    gtk_revealer_set_reveal_child(window->shred_revealer, TRUE);
    gtk_revealer_set_reveal_child(window->abort_revealer, FALSE);

    gtk_widget_set_sensitive(GTK_WIDGET(window->clear_button), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(window->shred_button), TRUE);
    gtk_button_set_label(window->clear_button, _("Clear All"));
    gtk_button_set_label(window->shred_button, _("Shred All"));

    // Get the result
    gboolean success = corrupt_start_shredding_finish(corrupt, res, &error);

    if (!success)
    {
        g_printerr("Failed to corrupt: %s\n", error->message);
        g_error_free(error);

        // TODO: Add toast notification.
    }
    else
    {
        // TODO: Add toast notification.
    }
}

static void raider_window_start_shredding(GtkWidget *widget, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    gtk_revealer_set_reveal_child(window->open_revealer, FALSE);
    gtk_revealer_set_reveal_child(window->shred_revealer, FALSE);
    gtk_revealer_set_reveal_child(window->abort_revealer, TRUE);

    gtk_button_set_label(window->shred_button, _("Starting Shredding…"));

    window->status = TRUE;

    corrupt_start_shredding_async(window->corrupt, window->cancel_shredding, raider_window_shred_files_finish, window);
}

/******** Asynchronously abort shredding on all files.  *********/
static void raider_window_abort_files_finish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    RaiderWindow *window = RAIDER_WINDOW(source_object);

    if (g_strcmp0((gchar *)user_data, "exit") == 0)
    {
        gtk_window_destroy(GTK_WINDOW(window));
    }

    /* Update the header bar view. */
    gtk_revealer_set_reveal_child(window->shred_revealer, TRUE);
    gtk_revealer_set_reveal_child(window->abort_revealer, FALSE);
    gtk_revealer_set_reveal_child(window->open_revealer, TRUE);

    window->status = FALSE;

    /* Revert the text and view of the abort button. */
    gtk_widget_set_sensitive(GTK_WIDGET(window->abort_button), TRUE);
    gtk_button_set_label(window->abort_button, _("Abort All"));
}
/* This is run asynchronously. */
static void raider_window_abort_files_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    // raider_window_abort_file_finish() is called here.
}
static void raider_window_abort_shredding(GtkWidget *widget, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    gtk_widget_set_sensitive(GTK_WIDGET(window->abort_button), FALSE);
    gtk_button_set_label(window->abort_button, _("Aborting…"));
    window->show_notification = FALSE;

    GTask *task = g_task_new(window, NULL, raider_window_abort_files_finish, data);
    g_task_run_in_thread(task, raider_window_abort_files_thread);
    g_object_unref(task);
}
/******** End of asynchronously abort shredding on all files section.  *********/
