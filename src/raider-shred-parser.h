#include "raider.h"
#include "raider-window.h"

void analyze_progress (GObject *source_object, GAsyncResult *res, gpointer user_data);
gboolean process_shred_output (gpointer data);
void stop (void *ptr_to_fsm);
void start (void *ptr_to_fsm);
void parse_sender_name (void *ptr_to_fsm);
void parse_filename (void *ptr_to_fsm);
void parse_pass(void *ptr_to_fsm);
void parse_fraction (void *ptr_to_fsm);
