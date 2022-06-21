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
#include "raider-progress-icon.h"

/* Parsing data that carries around data. */
struct _fsm
{
    void (*state)(void *);
    gchar **tokens;
    gint incremented_number;
    GtkWidget *progress_icon;
    GtkWidget *popover;
    gchar *filename;
    GSettings *settings;
    gdouble current;
    gdouble number_of_passes;
};

void parse_fraction(void *ptr_to_fsm);
void parse_pass(void *ptr_to_fsm);
void parse_filename(void *ptr_to_fsm);
void parse_sender_name(void *ptr_to_fsm);
void stop(void *ptr_to_fsm);
void start(void *ptr_to_fsm);
void parse_shred_data_type (void *ptr_to_fsm);
void parse_sub_percentage (void *ptr_to_fsm);

void analyze_progress(gchar* buffer, GtkWidget* progress_icon, GtkWidget* popover, gchar* filename, GSettings* settings)
{
	gchar **tokens = g_strsplit(buffer, " ", 0);
	struct _fsm fsm = { start, tokens, 0, progress_icon, popover, filename, settings };

	/* Pretty clever, no? */
	while (fsm.state != NULL) {
		fsm.state(&fsm);
	}

	g_free(tokens);
	g_free(buffer);
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
	for (i = 0; i < fsm->incremented_number; i++) {
		fsm->tokens--;
	}

	gdouble progress = (fsm->current / fsm->number_of_passes);
	raider_progress_icon_set_progress(RAIDER_PROGRESS_ICON(fsm->progress_icon), progress);
	//raider_progress_info_popover_set_progress(RAIDER_PROGRESS_INFO_POPOVER(fsm->popover), progress);
}

/* This function checks if shred sent the output. */
void parse_sender_name(void *ptr_to_fsm)
{
	struct _fsm *fsm = ptr_to_fsm;
	fsm->state = parse_filename;

	/* Get the path to 'shred' and compare. */
	gchar *path_to_shred = g_strconcat(g_settings_get_string(fsm->settings, "shred-executable"),
					   ":", NULL);


	if (g_strcmp0(path_to_shred, fsm->tokens[0]) != 0) {
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
	if (fsm->filename != NULL) {
		placeholder = g_strsplit(fsm->filename, " ", 0);
	}else {
		fsm->state = stop;
	}

	/* This is for if the filename has multiple spaces in it. */
	int number = 0;
	while (placeholder[number] != NULL) {
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
	if (g_strcmp0(_("pass"), fsm->tokens[0]) != 0) {
		fsm->state = stop;
		//g_printerr("Got '%s' instead of pass", fsm->tokens[0]);
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
	 * whereas the progress bar indicates how much more needs to be done. */
	fsm->current = g_strtod(fraction_chars[0], NULL) - 1;
	fsm->number_of_passes = g_strtod(fraction_chars[1], NULL);
	/* The progress update is done in the function stop. */

	g_free(fraction_chars);

	/* Set the text of the progress bar with a string that looks like '7/15". */
	//raider_progress_info_popover_set_text(RAIDER_PROGRESS_INFO_POPOVER(fsm->popover), fsm->tokens[0]);

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
