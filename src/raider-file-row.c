#include <gtk/gtk.h>
#include "raider-file-row.h"

struct _RaiderFileRow
{
    GtkListBoxRow parent;

    GtkWidget *box;
    GtkWidget *secondary_box;
    GtkWidget *filename_label;
    GtkWidget *remove_from_list_button;

    /* Progress bar widgets. */
    GtkWidget *revealer;
    GtkWidget *progress_bar;

    /* Data items. */
    gchar *filename;
    gchar *basename;
    GDataInputStream *data_stream;
};

G_DEFINE_TYPE (RaiderFileRow, raider_file_row, GTK_TYPE_LIST_BOX_ROW)

/* Parsing data that carries around data. */
struct _fsm
{
    void (*state)(void *);
    gchar **tokens;
    gint incremented_number;
    GtkWidget *progress_bar;
    gchar *filename;
};

void analyze_progress(GObject *source_object, GAsyncResult *res, gpointer user_data);
gboolean process_shred_output(gpointer data);
/*                                          */

void raider_file_row_delete (GtkWidget *widget, gpointer data)
{
    GtkWidget *widget2 = gtk_widget_get_parent(widget);
    GtkWidget *widget3 = gtk_widget_get_parent(widget2);
    GtkWidget *widget4 = gtk_widget_get_parent(widget3);
    gtk_widget_destroy(widget4);
}

static void
raider_file_row_init (RaiderFileRow *row)
{
    row->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (row), row->box);

    /* Create and add the secondary box which contains the remove button and the filename. */
    row->secondary_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(row->box), row->secondary_box, TRUE, TRUE, 0);

    /* Create the revealer container, which will hold the progress bar. */
    row->revealer = gtk_revealer_new();
    gtk_box_pack_start(GTK_BOX(row->box), row->revealer, TRUE, TRUE, 0);

    /* Add widgets to the secondary box. */
    row->filename_label = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(row->secondary_box), row->filename_label, TRUE, TRUE, 0);
    gtk_widget_set_halign(row->filename_label, GTK_ALIGN_START);

    row->remove_from_list_button = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(row->secondary_box), row->remove_from_list_button, TRUE, TRUE, 0);
    gtk_widget_set_halign(row->remove_from_list_button, GTK_ALIGN_END);
    g_signal_connect(row->remove_from_list_button, "clicked", G_CALLBACK(raider_file_row_delete), NULL);

    /* Add widget to revealer. */
    row->progress_bar = gtk_progress_bar_new();
    gtk_container_add(GTK_CONTAINER(row->revealer), row->progress_bar);
    gtk_revealer_set_reveal_child(GTK_REVEALER(row->revealer), FALSE);

    /* Last bit of work. */
    gtk_widget_show_all (GTK_WIDGET(row));
}

static void
raider_file_row_dispose (GObject *obj)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(obj);

    g_free(row->filename);
    row->filename = NULL;

    g_free(row->basename);
    row->basename = NULL;

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

    file_row->filename = g_strdup(str);
    file_row->basename = g_path_get_basename(str);

    /* Set the label's properties. */
    gtk_label_set_label(GTK_LABEL(file_row->filename_label), file_row->basename);
    gtk_widget_set_tooltip_text(file_row->filename_label, file_row->filename);

    return file_row;
}

/** Utility functions **/

/* This function which is call be g_thread_pool_push starts the shredding. */
void shredding_thread(gpointer data, gpointer user_data)
{
    RaiderFileRow *file_row = RAIDER_FILE_ROW(data);
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
    //if (removed_timeout == FALSE)
    //{
    //    g_printerr("Could not stop timeout.\n");
    //}

    /* Notify the user that it is done. */
    //gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(file_row->progress_bar), 1.0);
    raider_file_row_delete(data, NULL);
}

gboolean process_shred_output(gpointer data)
{
    /* Converting the stream to text. */
    RaiderFileRow *row = RAIDER_FILE_ROW(data);

    g_data_input_stream_read_line_async(row->data_stream, G_PRIORITY_DEFAULT, NULL, analyze_progress, data);
    return TRUE;
}

void analyze_progress(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(user_data);

    gchar *buf = g_data_input_stream_read_line_finish(row->data_stream, res, NULL, NULL);

    /* If there is no data read in or available, return immediately. */
    if (buf == NULL)
    {
        return;
    }

    /* Parse the line of text. */
    printf("%s\n", buf);

    /* The reason why I use a fsm style is because it allows redirects, and
    functions can be self contained, and not hold duplicate code. */

    /*gchar **tokens = g_strsplit(buf, " ", 0);
    struct _fsm fsm = {start, tokens, 0, pass_data->progress_bar, pass_data->filename};*/

    /* Pretty clever, no? */
    /*while (fsm.state != NULL)
    {
        fsm.state(&fsm);
    }

    g_free(tokens);*/
    g_free(buf);
}

/* Start the parsing. */
void start(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_sender_name;
}

/* Abort the while loop in analyze_progress. */
void stop(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = NULL;

    /* Reset the tokens array so it can be freed. */
    int i;
    for (i = 0; i < fsm->incremented_number; i++)
    {
        fsm->tokens--;
    }
}

/* This function checks if shred sent the output. */
void parse_sender_name(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_filename;

    if (g_strcmp0("/usr/bin/shred:", fsm->tokens[0]) != 0)
    {
        fsm->state = stop;
        g_printerr("No shred output found.");
        return;
    }

    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}

void parse_filename(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_pass;

    gchar **placeholder = g_strsplit(fsm->filename, " ", 0);

    /* This is for if the filename has multiple spaces in it. */
    int number = 0;
    while (placeholder[number] != NULL)
    {
        /* Point to the next word. */
        fsm->tokens++;
        fsm->incremented_number++;

        /* Update the number of spaces. */
        number++;
    }
    g_free(placeholder);
}

void parse_pass(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_fraction;

    if (g_strcmp0("pass", fsm->tokens[0]) != 0)
    {
        fsm->state = stop;
        g_printerr("Could not find correct text (wanted 'pass', got '%s').", fsm->tokens[0]);
        return;
    }

    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}

void parse_fraction(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = stop;

    gchar **fraction_chars = g_strsplit(fsm->tokens[0], "/", 0);

    /* THE BIG CODE OF SHREDDING PROGRESS. */
    int current = g_strtod(fraction_chars[0], NULL);
    int number_of_passes = g_strtod(fraction_chars[1], NULL);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(fsm->progress_bar), (gdouble)current / number_of_passes);

    g_free(fraction_chars);

    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}
