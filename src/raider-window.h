#ifndef __RAIDERWINDOW_H
#define __RAIDERWINDOW_H

#include "raider.h"
#include "raider-add-file.h"
#include "raider-shred-file.h"

#define RAIDER_WINDOW_TYPE (raider_window_get_type ())

G_DECLARE_FINAL_TYPE (RaiderWindow, raider_window, RAIDER, WINDOW, GtkApplicationWindow)

struct _RaiderWindow
{
    GtkApplicationWindow parent;

    GtkWidget *header_bar;
    GtkWidget *contents_box;
    GtkWidget *shred_button;
    GtkWidget *primary_menu;
    GtkWidget *tree_view;
    GtkWidget *hide_shredding_check_button;
    GtkWidget *number_of_passes_spin_button;
    GtkWidget *remove_file_check_button;
    GtkWidget *sample;
    GtkWidget *progress_overlay_revealer;
    GtkWidget *progress_overlay_scrolled_window;
    GtkWidget *tree_view_overlay_revealer;
    GtkWidget *progress_overlay_box;

    GSettings *g_settings;
    GtkSettings *gtk_settings;
    GtkListStore *list_store;
    GPtrArray *array_of_files;
    GPtrArray *array_of_progress_bars;
    GPtrArray *array_of_progress_labels;
    gint loaded_file_count;
    gint how_many_done;
};


RaiderWindow *raider_window_new (Raider *app);
void raider_window_open (RaiderWindow *win, GFile **files);
void shred_file(GtkWidget *widget, gpointer data);
void on_drag_data_received (GtkWidget *wgt, GdkDragContext *context, gint x, gint y,
                            GtkSelectionData *seldata, guint info, guint time,
                            gpointer data);
void open_file_dialog (GtkWidget *widget, gpointer data);

#endif /* __EXAMPLEAPPWIN_H */
