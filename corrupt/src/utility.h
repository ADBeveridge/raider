#ifndef CORRUPT_UTILITY_H
#define CORRUPT_UTILITY_H

#include <linux/magic.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <unordered_map>

struct strategy
{
public:
    int passes;
    const char *pattern;
    int pattern_len;
    bool verifyWrite;
    bool obfuscateFilename;
    int thread_count;

    std::string warning;
};

inline bool check_file(const char *filename)
{
    struct stat st;

    if (lstat(filename, &st) != 0)
    {
        fprintf(stderr, "corrupt: current file not found\n");
        return false;
    }
    if (S_ISLNK(st.st_mode) == 1)
    {
        fprintf(stderr, "corrupt: symbolic link cannot be processed\n");
        return false;
    }
    if (S_ISREG(st.st_mode) == 0)
    {
        fprintf(stderr, "corrupt: current file is not a regular file\n");
        return false;
    }
    return true;
}

inline const std::unordered_map<__fsword_t, strategy> fs_strategies = {
    {EXT4_SUPER_MAGIC, {3, "\x00\xFF\x55", 3, true, true, 4, ""}},
    {BTRFS_SUPER_MAGIC, {0, "", 0, false, false, 0, "Shredding not guaranteed for CoW filesystems"}},
};

inline struct strategy getStrategy(const char *filename)
{
    struct statfs fs_info;
    if (statfs(filename, &fs_info) == 0)
    {
        auto it = fs_strategies.find(fs_info.f_type);
        if (it != fs_strategies.end())
        {
            return it->second;
        }
    }

    struct strategy strat{
        .passes = 1,
        .pattern = "abc",
        .pattern_len = 3,
        .verifyWrite = false,
        .obfuscateFilename = false,
        .thread_count = 1,
        .warning = "Unknown FS"
    };
    return strat;
}

inline bool corrupt_pass(const char *filename, struct strategy &strat)
{
    if (strat.pattern_len == 0)
    {
        return true;
    }

    // Run some checks.
    FILE *fp = fopen(filename, "r+");
    if (fp == NULL)
    {
        return false;
    }

    // Get filesize.
    off_t size = 0;
    struct stat st;
    if (fstat(fileno(fp), &st) != 0)
    {
        return false;
    }
    size = st.st_size;

    const size_t buf_size = 65536; // 64 KB buffer
    char buffer[buf_size];

    for (size_t i = 0; i < buf_size; i++)
    {
        buffer[i] = strat.pattern[i % strat.pattern_len];
    }

    off_t bytes_written = 0;
    while (bytes_written < size)
    {
        size_t remaining = static_cast<size_t>(size - bytes_written);
        size_t chunk = remaining < buf_size ? remaining : buf_size;

        fwrite(buffer, sizeof(char), chunk, fp);
        bytes_written += chunk;
    }

    fflush(fp);
    fsync(fileno(fp));
    fclose(fp);
    return true;
}

inline bool corrupt_file(const char *filename, struct strategy &strat)
{
    bool res = check_file(filename);
    if (!res)
    {
        return false;
    }

    // Shred the file by overwriting it many times.
    for (int i = 0; i < strat.passes; i++)
    {
        if (corrupt_pass(filename, strat) == false)
        {
            fprintf(stderr, "corrupt: failed to perform shredding step\n");
            return false;
        }
    }

    if (remove(filename) != 0)
    {
        fprintf(stderr, "corrupt: failed to unlink file\n");
        return false;
    }

    return true;
}

#endif // CORRUPT_UTILITY_H
