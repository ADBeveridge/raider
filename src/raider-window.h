#ifndef __RAIDERWINDOW_H
#define __RAIDERWINDOW_H

#include <gtk/gtk.h>
#include "raider.h"

#define RAIDER_WINDOW_TYPE (raider_window_get_type ())

G_DECLARE_FINAL_TYPE (RaiderWindow, raider_window, RAIDER, WINDOW, GtkApplicationWindow)

RaiderWindow *raider_window_new (Raider *app);

void raider_window_open (gchar *filename_to_open, gpointer data);
void shred_file(GtkWidget *widget, gpointer data);
void on_drag_data_received (GtkWidget *wgt, GdkDragContext *context, gint x, gint y, GtkSelectionData *seldata, guint info, guint time, gpointer data);
void raider_window_open_file_dialog (GtkWidget *button, RaiderWindow *window);
void raider_window_close (gpointer data, gpointer user_data);

#endif /* __RAIDERWINDOW_H */
