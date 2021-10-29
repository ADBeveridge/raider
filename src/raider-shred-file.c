#include "raider-shred-file.h"


/* This function which is call be g_thread_pool_push starts the shredding. */
void thread_pool_function (gpointer data, gpointer user_data)
{
    RaiderWindow *window = RAIDER_WINDOW(user_data);

    GError *error = NULL;

    /* Determine the arguments. */
    gchar tmp[4];
    sprintf (tmp, "%d", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (window->number_of_passes_spin_button)));

    GSubprocess *process = g_subprocess_new(G_SUBPROCESS_FLAGS_STDERR_PIPE, &error,
                                            "/usr/bin/shred", "--verbose", (gchar *) data,
                                            g_strconcat ("--iterations=", tmp, NULL), /* This will always be put in. */
                                            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (window->remove_file_check_button) ) ? "--remove=wipesync" : "--verbose",
                                            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (window->hide_shredding_check_button) ) ? "--zero" : "--verbose",
                                            NULL);
    if (error != NULL)
    {
        g_error ("Process launching failed: %s", error->message);
        g_error_free(error);
        return;
    }

    GInputStream *stream = g_subprocess_get_stderr_pipe(process);

    /* This parses the output. */
    int timeout_id = g_timeout_add (500, process_shred_output, stream);

    /* Block this threaded function till it returns. */
    g_subprocess_wait(process, NULL, &error);

    /* When it is done, destroy the callback. We do not need it any longer. */
    gboolean removed_timeout = g_source_remove(timeout_id);
    if (removed_timeout == FALSE)
    {
        g_printerr("Could not stop timeout.\n");
    }
}

/*
 * THE BIG FUNCTION
 *
 * This is called when the user clicks on the Shred button in the options pane.
 */
void shred_file (GtkWidget *widget, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    /* Now let's show the progress screen. */
    gtk_revealer_set_reveal_child(GTK_REVEALER(window->progress_overlay_revealer), TRUE);
    gtk_revealer_set_reveal_child(GTK_REVEALER(window->tree_view_overlay_revealer), FALSE);

    if (window->loaded_file_count == 0)
    {
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window->header_bar), "No files loaded!");
        //return;
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
        gchar *tmp = g_ptr_array_index (window->array_of_files, iter);
        g_thread_pool_push(pool, g_strdup(tmp), NULL);
    }
}

void increment_number_of_subprocesses_finished (GPid pid, gint status, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);
    static int how_many_done = 0;

    how_many_done++;
    if (how_many_done == window->loaded_file_count)
    {
        GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                            GTK_DIALOG_DESTROY_WITH_PARENT,
                            GTK_MESSAGE_INFO,
                            GTK_BUTTONS_OK,
                            "Shredding finished" );

        gtk_window_set_title (GTK_WINDOW (dialog), "Message");
        gtk_dialog_run (GTK_DIALOG (dialog) );
        gtk_widget_destroy (dialog);
    }
    g_spawn_close_pid (pid);
}
