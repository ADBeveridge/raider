#include <gtk/gtk.h>
#include "raider-file-row.h"

struct _RaiderFileRow
{
    GtkListBoxRow parent;

    GtkWidget *row_box;
    GtkWidget *filename_label;
    GtkWidget *remove_from_list_button;

    /* Data items. */
    gchar *filename;
    GDataInputStream *data_stream;
};

G_DEFINE_TYPE (RaiderFileRow, raider_file_row, GTK_TYPE_LIST_BOX_ROW)

void raider_file_row_delete (GtkWidget *widget, gpointer data)
{
    GtkWidget *widget2 = gtk_widget_get_parent(widget);
    GtkWidget *widget3 = gtk_widget_get_parent(widget2);
    gtk_widget_destroy(widget3);
}

static void
raider_file_row_init (RaiderFileRow *row)
{
    row->row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    row->filename_label = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(row->row_box), row->filename_label, TRUE, TRUE, 0);
    gtk_widget_set_halign(row->filename_label, GTK_ALIGN_START);

    row->remove_from_list_button = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(row->row_box), row->remove_from_list_button, TRUE, TRUE, 0);
    gtk_widget_set_halign(row->remove_from_list_button, GTK_ALIGN_END);
    g_signal_connect(row->remove_from_list_button, "clicked", G_CALLBACK(raider_file_row_delete), NULL);

    gtk_container_add (GTK_CONTAINER (row), row->row_box);
}

static void
raider_file_row_dispose (GObject *obj)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(obj);

    g_free(row->filename);
    row->filename = NULL;

    G_OBJECT_CLASS (raider_file_row_parent_class)->dispose (obj);
}

static void
raider_file_row_class_init (RaiderFileRowClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = raider_file_row_dispose;
}

RaiderFileRow *raider_file_row_new (const char *str)
{
    RaiderFileRow *file_row = g_object_new (raider_file_row_get_type (), NULL);

    /* Make last-minute additions. */
    gtk_label_set_label(GTK_LABEL(file_row->filename_label), str);
    gtk_widget_show_all (GTK_WIDGET(file_row));
    file_row->filename = g_strdup(str);

    return file_row;
}

/** Utility functions **/

/* This function which is call be g_thread_pool_push starts the shredding. */
void shredding_thread(gpointer data, gpointer user_data)
{
    RaiderFileRow *file_row = RAIDER_FILE_ROW(user_data);
    GError *error = NULL;


    GSubprocess *process = g_subprocess_new(G_SUBPROCESS_FLAGS_STDERR_PIPE, &error,
                                            "/usr/bin/shred", "--verbose", file_row->filename,
                                            "--iterations=3",
                                            "--remove=wipesync",
                                            "--zero",
                                            NULL);
    if (error != NULL)
    {
        g_error("Process launching failed: %s", error->message);
        g_error_free(error);
        return;
    }

    /* This parses the output. */
    GInputStream *stream = g_subprocess_get_stderr_pipe(process);
    file_row->data_stream = g_data_input_stream_new(stream);

    /* Pass along the _pass_data struct again. */
    int timeout_id = g_timeout_add(100, process_shred_output, data);

    /* Block this threaded function till it returns. */
    g_subprocess_wait(process, NULL, &error);

    /* When it is done, destroy the callback. We do not need it any longer. */
    gboolean removed_timeout = g_source_remove(timeout_id);
    if (removed_timeout == FALSE)
    {
        g_printerr("Could not stop timeout.\n");
    }

    /* Notifify the user the it us done. */
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pass_data->progress_bar), 1.0);

    gchar *new_text = g_strconcat("Finished shredding ", pass_data->filename, NULL);
    gtk_label_set_text(GTK_LABEL(pass_data->progress_label), new_text);

    g_slice_free(struct _pass_data, pass_data);
    g_free(new_text);
}
