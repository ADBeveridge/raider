/* raider-window.c
 *
 * Copyright 2022 Alan
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
  GtkButton* open_button;
  GtkButton* shred_button;
  GtkButton* abort_button;

  GtkListBox* list_box;
};

G_DEFINE_TYPE(RaiderWindow, raider_window, GTK_TYPE_APPLICATION_WINDOW)

static void
raider_window_class_init(RaiderWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  gtk_widget_class_set_template_from_resource(widget_class, "/com/github/ADBeveridge/Raider/raider-window.ui");
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (widget_class), RaiderWindow, open_button);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (widget_class), RaiderWindow, open_button_full);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (widget_class), RaiderWindow, shred_button);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (widget_class), RaiderWindow, abort_button);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (widget_class), RaiderWindow, list_box);
}

static void
raider_window_init(RaiderWindow *self)
{
  gtk_widget_init_template(GTK_WIDGET(self));
}

