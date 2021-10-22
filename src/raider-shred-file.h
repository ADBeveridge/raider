#include "raider.h"
#include "raider-window.h"
#include "raider-shred-parser.h"

void increment_number_of_subprocesses_finished (GPid pid, gint status, gpointer data);
void shred_file (GtkWidget *widget, gpointer data);
