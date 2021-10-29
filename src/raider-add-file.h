#include "raider-window.h"


gboolean calculate_file_size (gchar *name, gpointer data, gchar result[]);
void open_file_selected (gchar *filename_to_open, gpointer data);
void open_file_dialog (GtkWidget *widget, gpointer data);
void on_drag_data_received (GtkWidget *wgt, GdkDragContext *context, gint x, gint y,
                            GtkSelectionData *seldata, guint info, guint time,
                            gpointer data);
gboolean check_file (gchar *filename, gpointer data);
