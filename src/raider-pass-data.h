#include "raider-window.h"

struct _pass_data
{
    RaiderWindow *window;
    GInputStream *stream;
    GDataInputStream *data_stream;
    GtkWidget *progress_bar; /* For easy access. */
    GtkWidget *progress_label;
    gchar *filename;
};
