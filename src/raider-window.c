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

#include <glib/gi18n.h>
#include <gio/gunixmounts.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "raider-config.h"
#include "raider-window.h"
#include "raider-file-row.h"

struct _RaiderWindow
{
    AdwApplicationWindow parent_instance;

    GtkBox *contents_box;
    GtkStack *window_stack;
    /* NOTE: NOT USED BECAUSE FLATPAK REMOVES ACCESS TO DEVICE FILES. */
    /*AdwSplitButton *open_button;*/
    GtkButton *open_button;
    GtkRevealer *open_revealer;
    GtkButton *shred_button;
    GtkRevealer *shred_revealer;
    GtkButton *abort_button;
    GtkRevealer *abort_revealer;
    AdwToastOverlay *toast_overlay;
    GtkListBox *list_box;

    GtkDropTarget *target;

    GList *filenames; // A quick list of filenames loaded for this window. */
    int file_count;
    gboolean status; // Shredding or not.

    /* NOTE: NOT USED BECAUSE FLATPAK REMOVES ACCESS TO DEVICE FILES. */
    // GMenu* mount_main_menu;
    // GMenu* mount_menu;
    // GVolumeMonitor* monitor;
};

G_DEFINE_TYPE(RaiderWindow, raider_window, ADW_TYPE_APPLICATION_WINDOW)

void raider_window_abort_shredding(GtkWidget *widget, gpointer data);

static void raider_window_class_init(RaiderWindowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    gtk_widget_class_set_template_from_resource(widget_class, "/com/github/ADBeveridge/Raider/raider-window.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, open_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, open_revealer);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, shred_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, abort_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, list_box);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, window_stack);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, shred_revealer);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, abort_revealer);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, toast_overlay);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, contents_box);
}

static void raider_window_exit_response(GtkDialog *dialog, gchar *response, RaiderWindow *self)
{
    if (g_strcmp0(response, "exit") == 0)
    {
        raider_window_abort_shredding(NULL, GTK_WIDGET(self));
    }
}

gboolean raider_window_exit(RaiderWindow *win, gpointer data)
{
    if (win->status)
    {
        GtkWidget *dialog = adw_message_dialog_new(GTK_WINDOW(win), _("Stop Shredding?"), NULL);
        adw_message_dialog_set_body(ADW_MESSAGE_DIALOG(dialog), _("Are you sure that you want to exit?"));
        g_signal_connect(dialog, "response", G_CALLBACK(raider_window_exit_response), win);

        adw_message_dialog_add_responses(ADW_MESSAGE_DIALOG(dialog), "cancel", _("_Cancel"), "exit", _("_Exit"), NULL);
        adw_message_dialog_set_response_appearance(ADW_MESSAGE_DIALOG(dialog), "exit", ADW_RESPONSE_DESTRUCTIVE);
        adw_message_dialog_set_default_response(ADW_MESSAGE_DIALOG(dialog), "cancel");
        adw_message_dialog_set_close_response(ADW_MESSAGE_DIALOG(dialog), "cancel");

        gtk_window_present(GTK_WINDOW(dialog));
    }

    return win->status;
}

void raider_window_show_toast(RaiderWindow *window, gchar *text)
{
    adw_toast_overlay_add_toast(window->toast_overlay, adw_toast_new(text));
}

/* Handle drop. */
static gboolean on_drop(GtkDropTarget *target, const GValue *value, double x, double y, gpointer data)
{
    /* GdkFileList is a boxed value so we use the boxed API. */
    GdkFileList *flist = g_value_get_boxed(value);

    /* Convert GSList to GList. */
    GSList *list = gdk_file_list_get_files(flist);
    GSList *l;
    GList *file_list = NULL;
    for (l = list; l != NULL; l = l->next)
    {
        file_list = g_list_append(file_list, g_file_dup(l->data));
    }

    raider_window_open_files(data, file_list);

    return TRUE;
}

/* This handles the application and window state. */
void raider_window_close_file(gpointer data, gpointer user_data, gint result)
{
    RaiderWindow *window = RAIDER_WINDOW(user_data);

    RaiderFileRow *row = RAIDER_FILE_ROW(data);
    gchar *filename = raider_file_row_get_filename(row);

    gboolean removed = FALSE;

    /* Search to delete the entry. */
    GList *item = window->filenames;
    while (item != NULL)
    {
        GList *next = item->next;

        /* Get the filename for this round. */
        gchar *text = (gchar *)item->data;
        if (g_strcmp0(text, filename) == 0)
        {
            window->filenames = g_list_remove(window->filenames, text);
            removed = TRUE;
        }
        item = next;
    }
    if (removed == FALSE)
        g_error(_("Could not remove filename from quick list. Please report this."));
    window->file_count--;

    if (window->file_count == 0)
    {
        gtk_stack_set_visible_child_name(window->window_stack, "empty_page");
        window->status = FALSE;

        if (result == 1)
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
    }
}

/********** File opening section. **********/

void raider_window_open_files_finish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
}

/* Asynchonous runner for raider_window_open_file. */
void raider_window_open_files_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    RaiderWindow *window = RAIDER_WINDOW(source_object);

    GList *file_list = task_data;
    GList *l;
    for (l = file_list; l != NULL; l = l->next)
    {
        gboolean cont = raider_window_open_file(l->data, window, NULL); // This adds an entry to the current window.
        if (cont == FALSE)
            break;
    }
    // raider_window_open_file_finish() is called here.
}

/* Start asynchronous loading of files. */
void raider_window_open_files(RaiderWindow *window, GList *file_list)
{
    GTask *task = g_task_new(window, NULL, raider_window_open_files_finish, window);
    g_task_set_task_data(task, file_list, NULL);
    g_task_run_in_thread(task, raider_window_open_files_thread);
    g_object_unref(task);
}

/* Systems have a limit on how many files can be open to a single process. */
rlim_t get_open_files_limit()
{
    struct rlimit limit;
    getrlimit(RLIMIT_NOFILE, &limit);
    return limit.rlim_cur;
}

/* This is used to open a single file at a time. Returns false if no more files can be loaded, true otherwise. */
gboolean raider_window_open_file(GFile *file, gpointer data, gchar *title)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    // Test if it exists.
    if (g_file_query_exists(file, NULL) == FALSE)
    {
        g_object_unref(file);
        return TRUE;
    }
    // Test if it a directory
    if (g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY)
    {
        gchar *message = g_strdup(_("Directories are not supported"));
        raider_window_show_toast(window, message);
        g_free(message);

        g_object_unref(file);
        return TRUE; // We can still load more files.
    }
    /* Search to see if a file with that path is already loaded. */
    GList *item = window->filenames;
    gchar *filename = g_file_get_path(file);
    while (item != NULL)
    {
        GList *next = item->next;

        /* Get the filename for this round. */
        gchar *text = (gchar *)item->data;

        if (g_strcmp0(text, filename) == 0)
        {
            gchar *message = g_strdup_printf(_("“%s” is already loaded"), g_file_get_basename(file));
            raider_window_show_toast(window, message);
            g_free(message);

            g_object_unref(file);
            return TRUE;
        }
        item = next;
    }
    g_list_free(item);
    /* Test if we can write. */
    if (g_access(g_file_get_path(file), W_OK) != 0)
    {
        gchar *message = g_strdup_printf(_("Cannot write to “%s”"), g_file_get_basename(file));
        raider_window_show_toast(window, message);
        g_free(message);

        g_object_unref(file);
        return TRUE;
    }
    /* The reason why i divide files limit by two is because the file count will be double when the processes are launched. */
    if (window->file_count >= (get_open_files_limit() / 2))
    {
        gchar *message = g_strdup(_("System cannot load more files"));
        raider_window_show_toast(window, message);
        g_free(message);

        g_object_unref(file);
        return FALSE; // No more files can be loaded.
    }

    /* We are OK then. */

    GtkWidget *file_row = GTK_WIDGET(raider_file_row_new(file));
    if (title)
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(file_row), title);
    gtk_list_box_append(window->list_box, file_row);

    gtk_stack_set_visible_child_name(GTK_STACK(window->window_stack), "list_page");
    gtk_revealer_set_reveal_child(GTK_REVEALER(window->shred_revealer), TRUE);

    window->file_count++;
    window->filenames = g_list_append(window->filenames, g_file_get_path(file));

    return TRUE;
}

/********** End of file opening section. **********/
/******** Asychronously launch shred on all files. *********/

void raider_window_shred_files_finish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    RaiderWindow *window = RAIDER_WINDOW(source_object);

    /* Update the view. */
    gtk_revealer_set_reveal_child(window->shred_revealer, FALSE);
    gtk_revealer_set_reveal_child(window->abort_revealer, TRUE);

    gtk_widget_set_sensitive(GTK_WIDGET(window->shred_button), TRUE);
    gtk_button_set_label(window->shred_button, _("Shred All"));
}

/* This is run asynchronously. */
void raider_window_shred_files_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    RaiderWindow *window = RAIDER_WINDOW(source_object);

    /* Launch the shredding. */
    int row;
    for (row = 0; row < window->file_count; row++)
    {
        RaiderFileRow *file_row = RAIDER_FILE_ROW(gtk_list_box_get_row_at_index(window->list_box, row));
        raider_file_row_launch_shredding((gpointer)file_row);
    }

    // raider_window_shred_file_finish() is called here.
}

/* Start the asynchronous shredding function. */
void raider_window_start_shredding(GtkWidget *widget, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    gtk_revealer_set_reveal_child(window->open_revealer, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(window->shred_button), FALSE);
    gtk_button_set_label(window->shred_button, _("Starting Shredding…"));

    window->status = TRUE;

    GTask *task = g_task_new(window, NULL, raider_window_shred_files_finish, window);
    g_task_run_in_thread(task, raider_window_shred_files_thread);
    g_object_unref(task);
}

/******** End of asychronously launch shred on all files section. *********/
/******** Asychronously abort shredding on all files.  *********/

void raider_window_abort_files_finish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    RaiderWindow *window = RAIDER_WINDOW(source_object);

    if (g_strcmp0((gchar *)user_data, "exit") == 0)
    {
        gtk_window_destroy(GTK_WINDOW(window));
    }

    /* Update the headerbar view. */
    gtk_revealer_set_reveal_child(window->shred_revealer, TRUE);
    gtk_revealer_set_reveal_child(window->abort_revealer, FALSE);
    gtk_revealer_set_reveal_child(window->open_revealer, TRUE);

    window->status = FALSE;

    /* Revert the text and view of the abort button. */
    gtk_widget_set_sensitive(GTK_WIDGET(window->abort_button), TRUE);
    gtk_button_set_label(window->abort_button, _("Abort All"));
}

/* This is run asynchronously. */
void raider_window_abort_files_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    RaiderWindow *window = RAIDER_WINDOW(source_object);

    /* Abort the shredding. */
    int row;
    for (row = 0; row < window->file_count; row++)
    {
        RaiderFileRow *file_row = RAIDER_FILE_ROW(gtk_list_box_get_row_at_index(window->list_box, row));
        raider_file_row_shredding_abort((gpointer)file_row);
    }
    // raider_window_abort_file_finish() is called here.
}

void raider_window_abort_shredding(GtkWidget *widget, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    gtk_widget_set_sensitive(GTK_WIDGET(window->abort_button), FALSE);
    gtk_button_set_label(window->abort_button, _("Aborting…"));

    gchar *datai;
    if (widget == NULL)
        datai = g_strdup("exit");
    else
    {
        datai = g_strdup("cont");
    }

    GTask *task = g_task_new(window, NULL, raider_window_abort_files_finish, datai);
    g_task_run_in_thread(task, raider_window_abort_files_thread);
    g_object_unref(task);
}

/******** End of asychronously abort shredding on all files section.  *********/

/* NOTE: NOT USED BECAUSE FLATPAK REMOVES ACCESS TO DEVICE FILES. */
/* Updates the list of removable media in the popover in the AdwSplitButton. */
/*void on_mount_changed(gpointer object, gpointer monitor, gpointer data)
{
    RaiderWindow *self = RAIDER_WINDOW(data);

    g_menu_remove_all(self->mount_menu);

    GList *mount_list = g_volume_monitor_get_mounts(self->monitor);
    GList *l;
    for (l = mount_list; l != NULL; l = l->next)
    {
        // Retrieve device path, and put in variant.
        GFile *file = g_mount_get_root(l->data); // Gets something like /home/ad/AD_BACKUPS.

        gchar *name = g_file_get_basename(file); // Get the "title" of the disk, something like AD_BACKUPS.
        GVariant *var = g_variant_new_string(g_file_get_path(file)); // Store in a variant so the "action" in raider-application can know which drive we are working on.

        GMenuItem *item = g_menu_item_new(name, "app.open-drive");
        g_menu_item_set_action_and_target_value(item, "app.open-drive", var);
        g_menu_append_item(self->mount_menu, item);

        g_object_unref(file);
        g_free(name);
    }
    if (g_list_length(mount_list) < 1)
        adw_split_button_set_menu_model(self->open_button, NULL);
    else
        adw_split_button_set_menu_model(self->open_button, G_MENU_MODEL(self->mount_main_menu));
}*/

static void raider_window_init(RaiderWindow *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));

    self->file_count = 0;
    self->filenames = NULL;
    self->status = FALSE;

    g_signal_connect(self->shred_button, "clicked", G_CALLBACK(raider_window_start_shredding), self);
    g_signal_connect(self->abort_button, "clicked", G_CALLBACK(raider_window_abort_shredding), self);
    g_signal_connect(self, "close-request", G_CALLBACK(raider_window_exit), NULL);

    /* Setup drag and drop. */
    self->target = gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_COPY);
    GType drop_types[] = {GDK_TYPE_FILE_LIST};
    gtk_drop_target_set_gtypes(self->target, drop_types, 1);
    g_signal_connect(self->target, "drop", G_CALLBACK(on_drop), self);
    gtk_widget_add_controller(GTK_WIDGET(self->contents_box), GTK_EVENT_CONTROLLER(self->target));

    /* NOTE: NOT USED BECAUSE FLATPAK REMOVES ACCESS TO DEVICE FILES. */
    /* Create monitor of mounted drives. */
    /*self->mount_main_menu = g_menu_new();
    self->mount_menu = g_menu_new();
    g_menu_prepend_section(self->mount_main_menu, _("Devices"), G_MENU_MODEL(self->mount_menu));
    adw_split_button_set_menu_model(self->open_button, G_MENU_MODEL(self->mount_main_menu));

    self->monitor = g_volume_monitor_get();
    g_signal_connect(self->monitor, "mount-added", G_CALLBACK(on_mount_changed), self);
    g_signal_connect(self->monitor, "mount-changed", G_CALLBACK(on_mount_changed), self);
    g_signal_connect(self->monitor, "mount-removed", G_CALLBACK(on_mount_changed), self);

    on_mount_changed(NULL, NULL, self);*/
}
