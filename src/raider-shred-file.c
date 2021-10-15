#include "raider-shred-file.h"

/* This function which is call be g_thread_pool_push starts the shredding. */
void thread_pool_function (gpointer data, gpointer user_data)
{
    RaiderWindow *window = RAIDER_WINDOW(user_data);

    GPid child_pid;
    GError *error = NULL;

    /* Determine the arguments. */
    gchar tmp[4];
    sprintf (tmp, "%d", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (window->number_of_passes_spin_button)));

    gchar *arguments[] = {raider->shred_path_option, "--verbose", (gchar *) data,
                          g_strconcat ("--iterations=", tmp, NULL), /* This will always be put in. */
                          gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (raider->remove_file_check_button) ) ? "--remove=wipesync" : "--verbose",
                          gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (raider->zeroed_pass_check_button) ) ? "--zero" : "--verbose",
                          NULL
                         };

    g_spawn_async (NULL, arguments, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &child_pid, &error);

    if (error != NULL)
    {
        g_error (_ ("Spawning child failed: %s"), error->message);
        g_error_free(error);
        return;
    }

    g_child_watch_add (child_pid, increment_number_of_subprocesses_finished, data);
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
