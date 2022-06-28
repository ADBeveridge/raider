/* raider-window.c
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

#include "raider-config.h"
#include "raider-window.h"
#include "raider-file-row.h"

struct _RaiderWindow {
	GtkApplicationWindow parent_instance;

	AdwSplitButton* open_button_full;
	GtkStack* window_stack;

	AdwSplitButton* open_button;
	GtkRevealer* open_revealer;
	GtkButton* shred_button;
	GtkRevealer* shred_revealer;
	GtkButton* abort_button;
	GtkRevealer* abort_revealer;

	AdwToastOverlay *toast_overlay;

	GtkListBox* list_box;
	int file_count;


};

G_DEFINE_TYPE(RaiderWindow, raider_window, GTK_TYPE_APPLICATION_WINDOW)

/*
 * THE BIG FUNCTION
 *
 * This function does not use a queue because shred is not cpu intensive, just disk intensive, and
   it is up to the disk to queue io requests.
 */
void shred_file(GtkWidget *widget, gpointer data)
{
	/* Clear the subtitle. */
	RaiderWindow *window = RAIDER_WINDOW(data);

	/* Update the headerbar view. */
	gtk_revealer_set_reveal_child(window->shred_revealer, FALSE);   // Hide.
	gtk_revealer_set_reveal_child(window->abort_revealer, TRUE);    // Show.
	gtk_revealer_set_reveal_child(window->open_revealer, FALSE);    // Hide.

	/* Launch the shredding. */
	int row;

	for (row = 0; row < window->file_count; row++) {
		RaiderFileRow* file_row = RAIDER_FILE_ROW(gtk_list_box_get_row_at_index(window->list_box, row));
		launch_shredding((gpointer)file_row);
	}
}

static void
raider_window_class_init(RaiderWindowClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	gtk_widget_class_set_template_from_resource(widget_class, "/com/github/ADBeveridge/Raider/raider-window.ui");
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, open_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, open_revealer);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, shred_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, abort_button);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, list_box);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, window_stack);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, shred_revealer);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, abort_revealer);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(widget_class), RaiderWindow, toast_overlay);
}

static void
raider_window_init(RaiderWindow *self)
{
	gtk_widget_init_template(GTK_WIDGET(self));


	self->file_count = 0;

	g_signal_connect(self->shred_button, "clicked", G_CALLBACK(shred_file), self);
}

/* This is called when the handler for the close button destroys the row. This handles the application state */
void raider_window_close(gpointer data, gpointer user_data)
{
	RaiderWindow *window = RAIDER_WINDOW(user_data);

	window->file_count--;

	if (window->file_count == 0) {
		gtk_stack_set_visible_child_name(window->window_stack, "empty_page");

		/* Update the view. */
		gtk_revealer_set_reveal_child(window->shred_revealer, FALSE);
		gtk_revealer_set_reveal_child(window->abort_revealer, FALSE);
		gtk_revealer_set_reveal_child(window->open_revealer, TRUE);

		//gtk_drag_dest_set (GTK_WIDGET(window), GTK_DEST_DEFAULT_ALL, &window->target_entry, 1, GDK_ACTION_COPY);
	}
}

/* This is used to open a single file at a time */
void raider_window_open(gchar *filename_to_open, gpointer data)
{
	RaiderWindow *window = RAIDER_WINDOW(data);

	GFile *file = g_file_new_for_path(filename_to_open);

	if (g_file_query_exists(file, NULL) == FALSE) {
		return;
	}
	if (g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY) {
		return;
	}
	g_object_unref(file);


	GtkWidget *file_row = GTK_WIDGET(raider_file_row_new(filename_to_open));

	g_signal_connect(file_row, "destroy", G_CALLBACK(raider_window_close), data);
	gtk_list_box_append(window->list_box, file_row);

	gtk_stack_set_visible_child_name(GTK_STACK(window->window_stack), "list_page");
	gtk_revealer_set_reveal_child(GTK_REVEALER(window->shred_revealer), TRUE);

	g_free(filename_to_open);

	window->file_count++;
}

