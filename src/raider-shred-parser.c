#include "raider-shred-parser.h"

/* ***************************************************************** */
/*                   Shred output it like this:                      */
/*                                                                   */
/*   shred: /home/pi/file-to-be-shredded.txt: pass 1/3 (random)...   */
/* ***************************************************************** */

struct _fsm
{
    void (*state) (void *);
    gchar **tokens;
    gint incremented_number;
};

void analyze_progress (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    gchar *buf = g_data_input_stream_read_line_finish(user_data, res, NULL, NULL);
    if (buf == NULL)
    {
        return;
    }

    /* Parse the line of text. */

    /* The reason why I use a fsm style is because it allows redirects, and
    functions can be self contained, and not hold duplicate code. */

    gchar **tokens =  g_strsplit(buf, " ", 0);
    struct _fsm fsm = {start, tokens, 0};

    /* Pretty clever, no? */
    while (fsm.state != NULL)
    {
        fsm.state(&fsm);
    }

    g_free(tokens);
}

gboolean process_shred_output (gpointer data)
{
    /* Converting the stream to text. */
    GDataInputStream *stream = g_data_input_stream_new(data);
    g_data_input_stream_read_line_async(stream, G_PRIORITY_DEFAULT,
                                        NULL, analyze_progress,
                                        stream);
    return TRUE;
}


/* Start the parsing. */
void start (void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_sender_name;
}

/* Abort the while loop in analyze_progress. */
void stop (void *ptr_to_fsm)
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
void parse_sender_name (void *ptr_to_fsm)
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

void parse_filename (void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_pass;

    /* TODO: Add checking for if this is the right file. */

    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}

void parse_pass (void *ptr_to_fsm)
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

void parse_fraction (void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = stop;

    gchar **fraction_chars = g_strsplit(fsm->tokens[0], "/", 0);

    /* The big code. This is sent to the progress bar. */
    int current = g_strtod(fraction_chars[0], NULL);
    int number_of_passes = g_strtod(fraction_chars[1], NULL);

    g_free(fraction_chars);

    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}
