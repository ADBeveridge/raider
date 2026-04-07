#ifndef CORRUPT_UTILITY_H
#define CORRUPT_UTILITY_H

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

strategy getStrategy(__fsword_t filesystem);
bool check_file(const char *filename);
bool corrupt_file(const char *filename, struct strategy strat);


#endif // CORRUPT_UTILITY_H
