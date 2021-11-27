#include "raider-shred-file.h"
#include "raider-pass-data.h"

/* This function which is call be g_thread_pool_push starts the shredding. */
void thread_pool_function(gpointer data, gpointer user_data)
{
    RaiderWindow *window = RAIDER_WINDOW(user_data);
    struct  _pass_data *pass_data = data;


    GError *error = NULL;

    /* Determine the arguments. */
    gchar tmp[4];
    sprintf(tmp, "%d", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(window->number_of_passes_spin_button)));

    GSubprocess *process = g_subprocess_new(G_SUBPROCESS_FLAGS_STDERR_PIPE, &error,
                                            "/usr/bin/shred", "--verbose", pass_data->filename,
                                            g_strconcat("--iterations=", tmp, NULL), /* This will always be put in. */
                                            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(window->remove_file_check_button)) ? "--remove=wipesync" : "--verbose",
                                            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(window->hide_shredding_check_button)) ? "--zero" : "--verbose",
                                            NULL);
    if (error != NULL)
    {
        g_error("Process launching failed: %s", error->message);
        g_error_free(error);
        return;
    }

    /* This parses the output. */
    pass_data->stream = g_subprocess_get_stderr_pipe(process);
    pass_data->data_stream = g_data_input_stream_new(pass_data->stream);

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

void launch (GtkWidget *widget, gpointer data)
{
    printf("Called once.\n");
}

/*
 * THE BIG FUNCTION
 *
 * This is called when the user clicks on the Shred button in the options pane.
 */
void shred_file(GtkWidget *widget, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    if (window->loaded_file_count == 0)
    {
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window->header_bar), "No files loaded!");
        return;
    }

    /* Clear the subtitle. */
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window->header_bar), NULL);

    /*
     * This is the actual code that launches shred. Make notice. It is the core of
     * the program.
     */

    GThreadPool *pool = g_thread_pool_new(thread_pool_function, data, 10, FALSE, NULL);

    int iter;
    for (iter = 0; iter < window->loaded_file_count; iter++)
    {
        //gchar *tmp = g_ptr_array_index(window->array_of_files, iter);

        /* Create the progress text. */
        //GtkWidget *text = gtk_label_new(tmp);
        //g_ptr_array_add(window->array_of_progress_labels, text);
        //gtk_container_add(GTK_CONTAINER(window->progress_overlay_box), text);
        //gtk_widget_show(text);

        /* Create the progress bar. */
        //GtkWidget *progress_bar = gtk_progress_bar_new();
        //g_ptr_array_add(window->array_of_progress_bars, progress_bar);
        //gtk_container_add(GTK_CONTAINER(window->progress_overlay_box), progress_bar);
        //gtk_widget_show(progress_bar);

        //struct _pass_data *pass_data = g_slice_new(struct _pass_data);
        //pass_data->progress_bar = progress_bar;
        //pass_data->progress_label = text;
        //pass_data->filename = g_strdup(tmp);

        //g_thread_pool_push(pool, pass_data, NULL);
        gtk_container_forall(GTK_CONTAINER(window->list_box), launch, NULL);
    }
}

void increment_number_of_subprocesses_finished(GPid pid, gint status, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);
    static int how_many_done = 0;

    how_many_done++;
    if (how_many_done == window->loaded_file_count)
    {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_INFO,
                                                   GTK_BUTTONS_OK,
                                                   "Shredding finished");

        gtk_window_set_title(GTK_WINDOW(dialog), "Message");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    g_spawn_close_pid(pid);
}
