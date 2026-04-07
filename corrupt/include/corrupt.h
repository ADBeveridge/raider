#pragma once

#include <time.h> /* time */
#include <stdio.h> /* FILE, fopen, fwrite, fclose, fprintf */
#include <stdint.h> /* uint*_t int*_t */
#include <string.h> /* strlen */
#include <stdlib.h> /* srand, rand */
#include <sys/stat.h> /* stat */
#include <sys/types.h> /* off_t */
#include <gtk/gtk.h> /* progress variables */

#define CORRUPT_TYPE (corrupt_get_type())

G_DECLARE_FINAL_TYPE(Corrupt, corrupt, CORRUPT, GObject)

bool corrupt_add_file(Corrupt *corrupt, const char *filename);
