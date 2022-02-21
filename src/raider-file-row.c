#include <gtk/gtk.h>
#include <handy.h>
#include <dazzle.h>
#include <glib/gi18n.h>
#include "raider-file-row.h"

struct _RaiderFileRow
{
    HdyActionRow parent;
	GtkWidget *button_box;

    GtkWidget *remove_from_list_button;
	GtkWidget *progress_button;
	GtkWidget *progress_icon;

    /* Notification widget. */
    GNotification *notification;
	gchar *notification_title;
	gchar *notification_subtitle;

    /* Data items. */
    GSettings *settings;
    gchar *filename;
    gchar *basename;
    GDataInputStream *data_stream;
    GSubprocess *process;
    guint signal_id;
    gint timout_id;
    gboolean aborted;
};

G_DEFINE_TYPE (RaiderFileRow, raider_file_row, HDY_TYPE_ACTION_ROW)

/* Parsing data that carries around data. */
struct _fsm
{
    void (*state)(void *);
    gchar **tokens;
    gint incremented_number;
    GtkWidget *progress_icon;
    gchar *filename;

    gdouble current;
    gdouble number_of_passes;
};

void analyze_progress(GObject *source_object, GAsyncResult *res, gpointer user_data);
gboolean process_shred_output(gpointer data);
void parse_fraction(void *ptr_to_fsm);
void parse_pass(void *ptr_to_fsm);
void parse_filename(void *ptr_to_fsm);
void parse_sender_name(void *ptr_to_fsm);
void stop(void *ptr_to_fsm);
void start(void *ptr_to_fsm);
void parse_shred_data_type (void *ptr_to_fsm);
void parse_sub_percentage (void *ptr_to_fsm);
/* End of parsing data.                     */

void raider_file_row_shredding_abort (GtkWidget *widget, gpointer data)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(data);
    row->aborted = TRUE;
    g_subprocess_force_exit(row->process);
}

void raider_file_row_delete (GtkWidget *widget, gpointer data)
{
	GtkWidget *list_box = gtk_widget_get_parent(data); /* Get the list box. */
    gtk_container_remove(GTK_CONTAINER(list_box), data);
}

static void
raider_file_row_init (RaiderFileRow *row)
{
	gtk_widget_init_template (GTK_WIDGET (row));

    row->settings = g_settings_new("com.github.ADBeveridge.Raider");
    row->aborted = FALSE;

    /* The remove button. */
	row->remove_from_list_button = gtk_button_new_from_icon_name ("edit-delete-symbolic", GTK_ICON_SIZE_BUTTON);
	row->signal_id = g_signal_connect (G_OBJECT(row->remove_from_list_button), "clicked", G_CALLBACK(raider_file_row_delete), row);
    gtk_box_pack_start (GTK_BOX(row->button_box), row->remove_from_list_button, TRUE, TRUE, 0);
	gtk_widget_show (row->remove_from_list_button);

    /* The progress button. */
	row->progress_button = gtk_button_new();
	gtk_widget_show(row->progress_button);

	row->progress_icon = dzl_progress_icon_new();
	gtk_button_set_image (GTK_BUTTON(row->progress_button), row->progress_icon);
	gtk_widget_show(row->progress_icon);
}

static void
raider_file_row_dispose (GObject *obj)
{
    RaiderFileRow *row = RAIDER_FILE_ROW(obj);
    g_free(row->filename);
    row->filename = NULL;
    g_free(row->basename);
    row->basename = NULL;
    g_free(row->notification_title);
    row->notification_title = NULL;
    g_free(row->notification_subtitle);
    row->notification_subtitle = NULL;

    G_OBJECT_CLASS (raider_file_row_parent_class)->dispose (obj);
}

static void
raider_file_row_class_init (RaiderFileRowClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = raider_file_row_dispose;

    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass), "/com/github/ADBeveridge/raider/ui/raider-file-row.ui");
    gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass), RaiderFileRow, button_box);
}

RaiderFileRow *raider_file_row_new (const char *str)
{
    RaiderFileRow *file_row = g_object_new (raider_file_row_get_type (), NULL);

    file_row->filename = g_strdup(str);
    file_row->basename = g_path_get_basename(str);

	g_object_set(file_row, "title", file_row->basename, NULL);
	hdy_action_row_set_subtitle (HDY_ACTION_ROW(file_row), file_row->filename);

    /* Notification stuff. */
    file_row->notification_title = g_strdup(_("Shredded"));
    file_row->notification = g_notification_new(file_row->notification_title);

    file_row->notification_subtitle = g_strdup(file_row->basename);
    g_notification_set_body(file_row->notification, file_row->notification_subtitle);

    return file_row;
}

/** Utility functions **/

void finish_shredding (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    RaiderFileRow *file_row = RAIDER_FILE_ROW(user_data);

    /* Remove the timeout. */
    gboolean removed_timeout = g_source_remove(file_row->timout_id);
    if (removed_timeout == FALSE)
    {
        g_printerr("Could not stop timeout.\n");
    }

    /* Send notification of completion. */
    if (file_row->aborted == FALSE)
    {
        GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET(file_row));
        if (!GTK_IS_WINDOW (toplevel)) return;
        GApplication *app = G_APPLICATION(gtk_window_get_application(GTK_WINDOW(toplevel)));
        g_application_send_notification(app, NULL, file_row->notification);
    }

    /* Remove the item. */
    raider_file_row_delete(NULL, file_row);
}

void launch (gpointer data, gpointer user_data)
{
    RaiderFileRow *file_row = RAIDER_FILE_ROW(data);
    GError *error = NULL;

    /* One liner settings. */
    gboolean remove_file = g_settings_get_boolean(file_row->settings, "remove-file");
    gboolean hide_shredding = g_settings_get_boolean(file_row->settings, "hide-shredding");
    gboolean override_permissions = g_settings_get_boolean(file_row->settings, "override-permissions");
    gboolean do_not_round_to_next_block = g_settings_get_boolean(file_row->settings, "do-not-round-to-next-block");
    gchar *shred_executable = g_settings_get_string(file_row->settings, "shred-executable");
    gboolean do_number_of_bytes_to_shred_command = TRUE;

    /* Get the number of passes. */
    gchar *number_of_passes = g_strdup_printf("%d", g_settings_get_int(file_row->settings, "number-of-passes"));
    gchar *number_of_passes_option = g_strconcat("--iterations=", number_of_passes, NULL);

    /* Get the remove method. The correspondings for each number is defined here. */
    gchar *remove_method;
    switch (g_settings_get_int (file_row->settings, "remove-method"))
        {
        case 0:
                {
                    remove_method = g_strdup("wipesync");
                    break;
                }
        case 1:
                {
                    remove_method = g_strdup("wipe");
                    break;
                }
        case 2:
                {
                    remove_method = g_strdup("unlink");
                    break;
                }
        }
    gchar *remove_method_command = g_strconcat("--remove=", remove_method, NULL);

    /* If that user requests, shred part of a file. */
    gchar *number_of_bytes_to_shred = g_settings_get_string(file_row->settings, "number-of-bytes-to-shred");
    if (g_strcmp0(number_of_bytes_to_shred, "") == 0) do_number_of_bytes_to_shred_command = FALSE;
    gchar *number_of_bytes_to_shred_command = g_strconcat("--size=", number_of_bytes_to_shred, NULL);

    /* Launch the shred executable on one file. There is a bit of a hack, as we substituted --verbose
    for the commands that are absent in this launch. There is no error as shred does not complain
    about too many --verbose'es */
    file_row->process = g_subprocess_new(G_SUBPROCESS_FLAGS_STDERR_PIPE, &error,
                                         shred_executable, "--verbose", file_row->filename,
                                         number_of_passes_option,
                                         remove_file ? remove_method_command : "--verbose",
                                         hide_shredding ? "--zero" : "--verbose",
                                         override_permissions ? "--force" : "--verbose",
                                         do_not_round_to_next_block ? "--exact" : "--verbose",
                                         do_number_of_bytes_to_shred_command ? number_of_bytes_to_shred_command : "--verbose",
                                         NULL);
    /* Free allocated text. */
    g_free(number_of_passes);
    g_free(number_of_passes_option);
    g_free(remove_method);
    g_free(remove_method_command);
    g_free(shred_executable);

    /* Avoid dangling pointer references. */
    number_of_passes = NULL;
    number_of_passes_option = NULL;
    remove_method = NULL;
    remove_method_command = NULL;
    shred_executable = NULL;

    if (error != NULL)
    {
        g_error("Process launching failed: %s", error->message);
        g_error_free(error);
        return;
    }

    /* This parses the output. */
    GInputStream *stream = g_subprocess_get_stderr_pipe(file_row->process);
    file_row->data_stream = g_data_input_stream_new(stream);

    /* Change the button. */
	gtk_widget_hide(file_row->remove_from_list_button);
	gtk_box_pack_start(GTK_BOX(file_row->button_box), file_row->progress_button, TRUE, TRUE, 0);

    /* Check the output every 100 milliseconds. */
    file_row->timout_id = g_timeout_add(100, process_shred_output, data);

    /* Call the callback when the process is finished. If the user aborts the
    the job, this will be called in any event.*/
    g_subprocess_wait_async(file_row->process, NULL, (GAsyncReadyCallback) finish_shredding, file_row);
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

    /* The reason why I use a fsm style is because it allows redirects, and
    functions can be self contained, and not hold duplicate code. */

    gchar **tokens = g_strsplit(buf, " ", 0);
    struct _fsm fsm = {start, tokens, 0, row->progress_icon, row->filename};

    /* Pretty clever, no? */
    while (fsm.state != NULL)
    {
        fsm.state(&fsm);
    }

    g_free(tokens);
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

    dzl_progress_icon_set_progress(DZL_PROGRESS_ICON(fsm->progress_icon), (gdouble) (fsm->current / fsm->number_of_passes));
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

    /* In this function, we divide the filename in the fsm struct.
       We then loop that double dimensional array till we reach the
       element that is null (the last one). Inside that loop we increment
       the pointer to the outputted text. This is done because some
       files have spaces in their filename. */

    gchar **placeholder;
    if (fsm->filename != NULL)
    {
        placeholder = g_strsplit(fsm->filename, " ", 0);
    }
    else
    {
        fsm->state = stop;
    }

    /* This is for if the filename has multiple spaces in it. */
    int number = 0;
    while (placeholder[number] != NULL)
    {
        /* Point to the next word for how may spaces there are in the filename. */
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

    /* Generally this test case will execute if the shredding option is set to remove. */
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
    fsm->state = parse_shred_data_type;

    gchar **fraction_chars = g_strsplit(fsm->tokens[0], "/", 0);

    /* THE BIG CODE OF SHREDDING PROGRESS. */
    /* I subtract one because in the output, it represents which pass it is working on,
     * whereas the progress bar indicates how much needs to be done. */
    fsm->current = g_strtod(fraction_chars[0], NULL) - 1;
    fsm->number_of_passes = g_strtod(fraction_chars[1], NULL);
    /* The progress update is done in the function stop. */

    g_free(fraction_chars);

    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}

void parse_shred_data_type (void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = stop;

    gchar **last_bit = g_strsplit(fsm->tokens[0], "...", 0);

    /* last_bit[0] contains the shred data type. If often contains (random) or (00000). */

    /* Check if this is such a big file that it gives more info. */
    if (g_strcmp0 (last_bit[1], "") != 0)
    {
        /* In this case, last_bit[1] contains something like "24MiB/83MiB". */
        /* We do not need it as the next token has the percentage of that pass is done. */
        fsm->state = parse_sub_percentage;
    }

    g_free(last_bit);

    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}

void parse_sub_percentage (void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = stop;

    /* Convert into whole number. The function ignores the % at the end of the digit. */
    gdouble percentage = g_strtod (fsm->tokens[0], NULL);

    /* Convert the percentage into a true decimal. */
    percentage = (percentage * .01);

    /* Update the current pass percentage because we have more info. */
    fsm->current = fsm->current + percentage;

    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}
