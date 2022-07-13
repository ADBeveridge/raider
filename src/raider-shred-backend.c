/* raider-shred-backend.c
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "raider-shred-backend.h"

/* Parsing data that carries around data. */
struct _fsm
{
    void (*state)(void *);
    gchar **tokens;
    gint incremented_number;
    gchar *filename;
    GSettings *settings;
    gdouble current;
    gdouble number_of_passes;

	gdouble* progress; // This is passed so it can be set.
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

struct _RaiderShredBackend {
	GObject parent;

	GDataInputStream *data_stream;
	gchar* filename;
	GSettings* settings;

	GTimer* timer;
	gint timeout_id;
	gdouble progress;
	gdouble rate;
};

G_DEFINE_TYPE(RaiderShredBackend, raider_shred_backend, G_TYPE_OBJECT )

enum {
	PROP_0,
	PROP_DATA_STREAM,
	PROP_FILENAME,
	PROP_SETTINGS,
	N_PROPS
};

static GParamSpec *properties[N_PROPS];

/* Return a property in a GValue passed along. */
static void raider_shred_backend_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	RaiderShredBackend *self = RAIDER_SHRED_BACKEND(object);

	switch (prop_id) {
	case PROP_DATA_STREAM:
		g_value_set_object(value, self->data_stream);
		break;

	case PROP_SETTINGS:
		g_value_set_object(value, self->settings);
		break;

	case PROP_FILENAME:
		g_value_set_string(value, self->filename);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void raider_shred_backend_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	RaiderShredBackend *self = RAIDER_SHRED_BACKEND(object);

	switch (prop_id) {
	case PROP_DATA_STREAM:
		self->data_stream = g_value_get_object(value);
		break;

	case PROP_SETTINGS:
		self->settings = g_value_get_object(value);
		break;

	case PROP_FILENAME:
		self->filename = g_strdup(g_value_get_string(value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void raider_shred_backend_dispose(GObject *obj)
{
    RaiderShredBackend* backend = RAIDER_SHRED_BACKEND(obj);

	/* Remove the timeout. */
    gboolean removed_timeout = g_source_remove(backend->timeout_id);
    if (removed_timeout == FALSE)
    {
        g_printerr("Could not stop timeout.\n");
    }
}

static void raider_shred_backend_class_init(RaiderShredBackendClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->get_property = raider_shred_backend_get_property;
	object_class->set_property = raider_shred_backend_set_property;
	object_class->dispose = raider_shred_backend_dispose;

	properties[PROP_DATA_STREAM] =
		g_param_spec_object("data-stream",
				    "Data Stream",
				    "The input source for output from a cl program",
				    G_TYPE_OBJECT,
				    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	properties[PROP_SETTINGS] =
		g_param_spec_object("settings",
				    "Settings",
				    "The GSettings object, used for verifing the program name",
				    G_TYPE_OBJECT,
				    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	properties[PROP_FILENAME] =
		g_param_spec_string("filename",
				    "Filename",
				    "The filename of the file that is being shredded",
				    NULL,
				    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_properties(object_class, N_PROPS, properties);
}


void raider_shred_backend_process_output_finish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	RaiderShredBackend *backend = RAIDER_SHRED_BACKEND(user_data);

	gchar *buf = g_data_input_stream_read_line_finish(backend->data_stream, res, NULL, NULL);

	/* If there is no data read in or available, return immediately. */
	if (buf == NULL) {
		return;
	}

	gchar **tokens = g_strsplit(buf, " ", 0);
	struct _fsm fsm = { start, tokens, 0, backend->filename, backend->settings, 0, 0, &backend->progress };
	while (fsm.state != NULL) {
		fsm.state(&fsm);
	}

	backend->rate = backend->progress / g_timer_elapsed(backend->timer, NULL); // Returns the number of milliseconds it takes to go up by one progress unit.

	g_free(tokens);
	g_free(buf);
}

void on_timeout_finished(gpointer user_data)
{
}

/* Start the read of the output. */
gboolean raider_shred_backend_process_output(gpointer data)
{
	/* Converting the stream to text. */
	RaiderShredBackend *backend = RAIDER_SHRED_BACKEND(data);
	g_data_input_stream_read_line_async(backend->data_stream, G_PRIORITY_DEFAULT, NULL, raider_shred_backend_process_output_finish, data);
	return TRUE;
}

static void raider_shred_backend_init(RaiderShredBackend *backend)
{
	backend->settings = g_settings_new("com.github.ADBeveridge.Raider");
	backend->progress = 0.0;

  	backend->timer = g_timer_new();
	g_timer_start(backend->timer);

	/* Check the output every 1/10th of a second. */
	backend->timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 500, raider_shred_backend_process_output, (gpointer)backend, on_timeout_finished);
}

gdouble raider_shred_backend_get_progress (RaiderShredBackend* backend)
{
	return backend->progress;
}


/** Parsing functions., **/


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

    *fsm->progress = (fsm->current / fsm->number_of_passes);
}

/* This function checks if shred sent the output. */
void parse_sender_name(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_filename;

    /* Get the path to 'shred' and compare. */
    gchar *path_to_shred = g_strconcat (g_settings_get_string(fsm->settings,
															  "shred-executable"),
										":", NULL);


    if (g_strcmp0(path_to_shred, fsm->tokens[0]) != 0)
    {
        fsm->state = stop;
        g_printerr("No shred output found.");
		g_free(path_to_shred);
        return;
    }

	g_free(path_to_shred);

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

    /* The actual comparison. This is for if the filename has multiple spaces in it. */
    int number = 0;
    while (placeholder[number] != NULL)
    {
		// TODO: Actually check if the filename is the same as the one given. */

        /* Point to the next word for how may spaces there are in the filename. */
        fsm->tokens++;
        fsm->incremented_number++;

        /* Update the number of spaces. */
        number++;
    }
    g_free(placeholder);
}

/* Simply checks this token for 'pass'. */
void parse_pass(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_fraction;

    /* Generally this test case will execute if the shredding option is set to remove. */
    if (g_strcmp0(_("pass"), fsm->tokens[0]) != 0)
    {
        fsm->state = stop;
        //g_printerr("Got '%s' instead of pass", fsm->tokens[0]);
        return;
    }
    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}

/* Parses the fraction that is right after the 'pass' word. */
void parse_fraction(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_shred_data_type;

    gchar **fraction_chars = g_strsplit(fsm->tokens[0], "/", 0);

    /* THE BIG CODE OF SHREDDING PROGRESS. */
    /* I subtract one because in the output, it represents which pass it is working on,
     * whereas the progress bar indicates how much more needs to be done. */
    fsm->current = g_strtod(fraction_chars[0], NULL) - 1;

	/* Then grab the total number of passes to be done. */
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

