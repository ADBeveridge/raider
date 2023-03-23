#pragma once

#include <time.h> /* time */
#include <stdio.h> /* FILE, fopen, fwrite, fclose, fprintf */
#include <stdint.h> /* uint*_t int*_t */
#include <string.h> /* strlen */
#include <stdlib.h> /* srand, rand */
#include <sys/stat.h> /* stat */
#include <sys/types.h> /* off_t */
#include <gtk/gtk.h>

uint8_t corrupt_file(const char *filename, GTask* task);
uint8_t corrupt_unlink_file(const char *filename);

