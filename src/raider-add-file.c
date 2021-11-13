#include "raider-add-file.h"

void open_file_dialog (GtkWidget *widget, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    GtkWidget *dialog = gtk_file_chooser_dialog_new ("Open File", GTK_WINDOW (window),
                        GTK_FILE_CHOOSER_ACTION_OPEN, "Cancel", GTK_RESPONSE_CANCEL, "Open",
                        GTK_RESPONSE_ACCEPT, NULL); /* Create dialog. */

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE); /* Alow to select many files at once. */

    gint res = gtk_dialog_run (GTK_DIALOG (dialog) );
    if (res == GTK_RESPONSE_ACCEPT)
    {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);

        GSList *list = gtk_file_chooser_get_filenames (chooser);
        g_slist_foreach (list, (GFunc) open_file_selected, data);
        g_slist_free (list);
    }

    gtk_widget_destroy (dialog);
}


/*
 *  This is called by show_open_file_dialog and from DND.
 */
void open_file_selected (gchar *filename_to_open, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    gboolean result = check_file (filename_to_open, data);
    if (result == FALSE)
    {
        return;
    }

    gchar file_size[40];
    gboolean is_calculated = calculate_file_size (filename_to_open, data, file_size);
    if (is_calculated == FALSE) /* If unable still exists. */
    {
        return;
    }

    //gchar *absolute_filename = g_path_get_basename (filename_to_open);
    //gchar *absolute_path = g_path_get_dirname (filename_to_open)
    //gchar *strdup_filename = g_strdup (filename_to_open);

    GtkWidget *file_row = GTK_WIDGET(raider_file_row_new(filename_to_open));
    gtk_container_add(GTK_CONTAINER (window->list_box), file_row);

    //g_ptr_array_add (window->array_of_files, strdup_filename);  //This value is freed when reiniting raider.

    window->loaded_file_count++;  // Update the count of files. */
    g_free (filename_to_open);
}

/*
 * This is called to calculate file size.
 */
gboolean calculate_file_size (gchar *name, gpointer data, gchar result[])
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    GError *error = NULL;
    GFile *file = g_file_new_for_path (name);
    GFileInfo *file_info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                           G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &error);

    if (error != NULL)
    {
        g_printerr ("%s", error->message);
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window->header_bar), "Cannot get file size!");
        return FALSE;
    }
    strcpy (result, g_file_info_get_attribute_as_string (file_info, G_FILE_ATTRIBUTE_STANDARD_SIZE) );
    strcat (result, " B" ); /* Abbr. for bytes. */

    return TRUE;
}


/*
 * This is called to make sure the file exists.
 */
gboolean check_file (gchar *filename, gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    GFile *file = g_file_new_for_path (filename);
    gboolean exists = g_file_query_exists (file, NULL);

    if (exists == FALSE)
    {
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window->header_bar), "File does not exist!");
    }

    return exists;
}

/*
 * This is called when the user drops files into the tree view.
 */
void on_drag_data_received (GtkWidget *wgt, GdkDragContext *context, gint x, gint y,
                            GtkSelectionData *seldata, guint info, guint time,
                            gpointer data)
{
    RaiderWindow *window = RAIDER_WINDOW(data);

    gchar **filenames = NULL;
    filenames = g_uri_list_extract_uris ( (const gchar *) gtk_selection_data_get_data (seldata) );

    if (filenames == NULL)
    {
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window->header_bar), "Unable to complete Drag and Drop!");

        gtk_drag_finish (context, FALSE, FALSE, time);
        return;
    }

    int iter = 0;

    while (filenames[iter] != NULL)
    {
        char *filename = g_filename_from_uri (filenames[iter], NULL, NULL);
        open_file_selected(filename, data);

        iter++;
    }

    gtk_drag_finish (context, TRUE, FALSE, time);
}
