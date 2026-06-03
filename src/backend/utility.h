#ifndef CORRUPT_UTILITY_H
#define CORRUPT_UTILITY_H

#include <stdbool.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct _FilePayload FilePayload;

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

bool corrupt_file(FilePayload *payload, strategy *strat);

bool check_file(const char *filename);
strategy getStrategy(const char *filename);

#endif // CORRUPT_UTILITY_H
