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
struct _fsm {
	// Set upon creation.
  	void (*state)(void *);
	gchar **tokens;
  	gchar *filename;
  	GSettings *settings;
  	gdouble* progress; // This is passed so it can be set.
  	gchar **shred_state; // NULL if okay.

  	// Internal variables.
	gint incremented_number;
  	gdouble current;
	gdouble number_of_passes;
};

void analyze_progress(GObject *source_object, GAsyncResult *res, gpointer user_data);
gboolean process_shred_output(gpointer data);
void parse_fraction(void *ptr_to_fsm);
void parse_indicator_token(void *ptr_to_fsm);
void parse_filename(void *ptr_to_fsm);
void parse_sender_name(void *ptr_to_fsm);
void parse_error(void *ptr_to_fsm);
void stop(void *ptr_to_fsm);
void start(void *ptr_to_fsm);
void parse_shred_data_type(void *ptr_to_fsm);
void parse_sub_percentage(void *ptr_to_fsm);
/* End of parsing data.                     */

struct _RaiderShredBackend {
	GObject parent;

	GDataInputStream *data_stream;
	gchar* filename;
	GSettings* settings;
  	gchar* shred_state;
	GTimer* timer;
	GTimer* smooth_timer; // Used for smooth progress tracking.
	gint timeout_id;
	gdouble progress;
	gdouble rate;

	struct _fsm fsm;
};

G_DEFINE_TYPE(RaiderShredBackend, raider_shred_backend, G_TYPE_OBJECT)

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

void raider_shred_backend_dispose(GObject *obj)
{
	RaiderShredBackend* backend = RAIDER_SHRED_BACKEND(obj);

	g_timer_destroy(backend->timer);
	if (backend->smooth_timer != NULL) g_timer_destroy(backend->smooth_timer);

	/* Remove the timeout. */
	gboolean removed_timeout = g_source_remove(backend->timeout_id);
	if (removed_timeout == FALSE) {
		g_printerr("Could not stop timeout.\n");
	}

	G_OBJECT_CLASS(raider_shred_backend_parent_class)->dispose(obj);
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

/* The regular way of reading the output of shred. It only parses a single line. */
void raider_shred_backend_process_output_finish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	RaiderShredBackend *backend = RAIDER_SHRED_BACKEND(user_data);

	gchar *buf = g_data_input_stream_read_line_finish(backend->data_stream, res, NULL, NULL);

	/* If there is no data read in or available, return immediately. */
	if (buf == NULL) {
		return;
	}

	gchar **tokens = g_strsplit(buf, " ", 0);
	struct _fsm fsm = { start, tokens, backend->filename, backend->settings, &backend->progress, &backend->shred_state};
	while (fsm.state != NULL) {
		fsm.state(&fsm);
	}

	backend->rate = backend->progress / g_timer_elapsed(backend->timer, NULL); // Returns the progress accoplished in one millisecond.

	g_free(tokens);
	g_free(buf);
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
	backend->shred_state = NULL;

	backend->timer = g_timer_new();
	g_timer_start(backend->timer);
	backend->smooth_timer = NULL;

	/* Check the output every 1/10th of a second. */
	backend->timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 500, raider_shred_backend_process_output, (gpointer) backend, NULL);
}

gdouble raider_shred_backend_get_progress(RaiderShredBackend* backend)
{
	/* This executes on the very first call to the progress getter. */
	if (backend->smooth_timer == NULL) {
		backend->smooth_timer = g_timer_new();
		g_timer_start(backend->smooth_timer);
	}

	gdouble progress = g_timer_elapsed(backend->smooth_timer, NULL) * backend->rate;
	return progress;
}

gchar* raider_shred_backend_get_return_result_string (RaiderShredBackend* backend)
{
	return backend->shred_state;
}

void raider_shred_backend_get_return_result_thread (GTask* task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	RaiderShredBackend* backend = RAIDER_SHRED_BACKEND (task_data);
	gchar* last = NULL;
	gchar* current = g_data_input_stream_read_line(backend->data_stream, NULL, NULL, NULL);

	// Loop through all the
	while(current != NULL)
	{
		if (last != NULL) g_free(last); // Free it.
		last = current;
		current = g_data_input_stream_read_line(backend->data_stream, NULL, NULL, NULL);
	}

	if (last == NULL) {
		// We already have the return message.
		return; // The callback specified by the get_return_result function is called here.
	}

	// Otherwise, we need to grab the return message.
	gchar **tokens = g_strsplit(last, " ", 0);
	struct _fsm fsm = { start, tokens, backend->filename, backend->settings, &backend->progress, &backend->shred_state};
	while (fsm.state != NULL) {
		fsm.state(&fsm);
	}
	g_free(last);

}

/* This function returns immediately, launching a g_task to read the output one final time. */
void raider_shred_backend_get_return_result(gpointer object, GAsyncReadyCallback callback, gpointer data)
{
  	GTask* task = g_task_new (NULL, NULL, callback, object);
	g_task_set_task_data (task, data, NULL);
  	g_task_run_in_thread (task, raider_shred_backend_get_return_result_thread);
  	g_object_unref (task);
}

/* Parsing functions., */
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
	for (i = 0; i < fsm->incremented_number; i++) {
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
	gchar *path_to_shred = g_strconcat(g_settings_get_string(fsm->settings,
								 "shred-executable"),
					   ":", NULL);


	if (g_strcmp0(path_to_shred, fsm->tokens[0]) != 0) {
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
	fsm->state = parse_indicator_token;

	/* In this function, we divide the filename in the fsm struct.
	   We then loop that double dimensional array till we reach the
	   element that is null (the last one). Inside that loop we increment
	   the pointer to the outputted text. This is done because some
	   files have spaces in their filename. */

	gchar **placeholder;
	if (fsm->filename != NULL) {
		placeholder = g_strsplit(fsm->filename, " ", 0);
	}else {
		fsm->state = stop;
		return;
	}

	/* The actual comparison. This is for if the filename has multiple spaces in it. */
	int number = 0;
	while (placeholder[number] != NULL) {
		// TODO: Actually check if the filename is the same as the one given. */

		/* Point to the next word for how may spaces there are in the filename. */
		fsm->tokens++;
		fsm->incremented_number++;

		/* Update the number of spaces. */
		number++;
	}
	g_free(placeholder);
}

/* Simply checks this token. Usu. it is 'pass'. */
void parse_indicator_token(void *ptr_to_fsm)
{
	struct _fsm *fsm = ptr_to_fsm;

	if (g_strcmp0(_("failed"), fsm->tokens[0]) == 0) {
		fsm->state = parse_error;
	}
	else if (g_strcmp0(_("removed"), fsm->tokens[0]) == 0) {
		fsm->state = stop; // No more tokens.
		*fsm->shred_state = g_strdup("good");
		return;
	}
	else if (g_strcmp0(_("pass"), fsm->tokens[0]) == 0) {
		fsm->state = parse_fraction;
	}

	/* Point to the next word. */
	fsm->tokens++;
	fsm->incremented_number++;
}

void parse_error (void *ptr_to_fsm)
{
	struct _fsm *fsm = ptr_to_fsm;
	fsm->state = stop;

	// Go past the "to open for writing" words.
	int number = 0;
	for (number = 0; number < 4; number++) {
		// TODO: Actually check the error message.
          fsm->tokens++;
     	fsm->incremented_number++;
     }

	gchar* message = g_strdup(fsm->tokens[0]);
	fsm->tokens++;
	fsm->incremented_number++;

	while(fsm->tokens[0] != NULL)
	{
	    	gchar* tmp = g_strconcat(message, " ", fsm->tokens[0], NULL);
		g_free(message);
		message = tmp;

		// Point to the next word for how may spaces there are in the message.
		fsm->tokens++;
		fsm->incremented_number++;
	}

	*fsm->shred_state = message;

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

	g_free(fraction_chars);

	// Pretty safe to say.
	*fsm->shred_state = g_strdup("good");

	/* Point to the next word. */
	fsm->tokens++;
	fsm->incremented_number++;
}

void parse_shred_data_type(void *ptr_to_fsm)
{
	struct _fsm *fsm = ptr_to_fsm;
	fsm->state = stop;

	gchar **last_bit = g_strsplit(fsm->tokens[0], "...", 0);

	/* last_bit[0] contains the shred data type. If often contains (random) or (00000). */
	/* We don't need to check it for now. */

	/* Check if this is such a big file that it gives more info. */
	if (g_strcmp0(last_bit[1], "") != 0) {
		/* In this case, last_bit[1] contains something like "24MiB/83MiB". */
		/* We do not need it as the next token has the percentage of that pass is done. */
		fsm->state = parse_sub_percentage;
	}

	g_free(last_bit);

	/* Point to the next word. */
	fsm->tokens++;
	fsm->incremented_number++;
}

/* Called with enormous files. */
void parse_sub_percentage(void *ptr_to_fsm)
{
	struct _fsm *fsm = ptr_to_fsm;
	fsm->state = stop;

	/* Convert into whole number. The function ignores the % at the end of the digit. */
	gdouble percentage = g_strtod(fsm->tokens[0], NULL);

	/* Convert the percentage into a true decimal. */
	percentage = (percentage * .01);

	/* Update the current pass percentage because we have more info. */
	fsm->current = fsm->current + percentage;

	/* Point to the next word. */
	fsm->tokens++;
	fsm->incremented_number++;
}

