#include "corrupt.h"

static uint8_t corrupt_step(const char *filename, const off_t filesize, const char *pattern, GTask* task, int loop)
{
    uint8_t ret = 0;

    FILE* fp = fopen(filename,  "r+");
    if (g_task_return_error_if_cancelled (task)) {fclose(fp);return 1;}
    if (fp != NULL)
    {
        off_t i;
        uint8_t length = (uint8_t) strlen(pattern);
        if (length > 0)
        {
            off_t times = (filesize / length) + (filesize % length);
            for (i = 0; i < times; i++)
            {
                fwrite(pattern, sizeof(char), length, fp);
                if (g_task_return_error_if_cancelled (task)) {fclose(fp);return 1;}
            }
        }
        else
        {
            srand((unsigned int) time(NULL));
            for (i = 0; i < filesize; i++)
            {
                int n = rand();
                fwrite(&n, sizeof(char), 1, fp);
                if (g_task_return_error_if_cancelled (task)) {fclose(fp);return 1;}
            }
        }
        fclose(fp);
    }
    else
    {   
        ret = 1;
    }
    return ret;
}

uint8_t corrupt_file(const char *filename, GTask* task)
{
    uint8_t ret = 0;
    
    struct stat st;
    if(stat(filename, &st) == 0)
    {
        if (S_ISREG(st.st_mode) != 0)
        {
            off_t filesize = st.st_size;
            const char* steps[35] = {"", "", "", "", "\x55", "\xAA",
                                     "\x92\x49\x24", "\x49\x24\x92",
                                     "\x24\x92\x49", "\x00", "\x11",
                                     "\x22", "\x33", "\x44", "\x55",
                                     "\x66", "\x77", "\x88", "\x99",
                                     "\xAA", "\xBB", "\xCC", "\xDD",
                                     "\xEE", "\xFF", "\x92\x49\x24",
                                     "\x49\x24\x92", "\x24\x92\x49",
                                     "\x6D\xB6\xDB", "\xB6\xDB\x6D",
                                     "\xDB\x6D\xB6", "", "", "", ""};
            uint8_t i;
            for (i = 0; i < 35; i++)
            {
                if (corrupt_step(filename, filesize, steps[i], task, i) != 0)
                {
                    ret = 1;
                    break;
                }
                printf("%d\n", i);
            }
        }
        else
        {
            fprintf(stderr, "corrupt: '%s' is not a regular file\n", filename);
            ret = 1;
        }
    }  
    else
    {
        fprintf(stderr, "corrupt: '%s' not found\n", filename);
        ret = 1;
    }
    return ret;
}

uint8_t corrupt_unlink_file(const char *filename)
{
    uint8_t ret = 0;

    printf("Removing...\n");
    if (remove(filename) != 0)
    {
        ret = 1;
    }

    return ret;
}

