#ifndef CORRUPT_UTILITY_H
#define CORRUPT_UTILITY_H

#include <stdbool.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gio/gio.h>

typedef struct strategy
{
    int passes;
    const char *pattern;
    int pattern_len;
    bool verifyWrite;
    bool obfuscateFilename;
    int thread_count;

    const char *warning;
} strategy;

strategy getStrategy(const char *filename);
bool check_file(const char *filename);
off_t get_folder_size_recursive(GFile *folder);

#endif // CORRUPT_UTILITY_H
