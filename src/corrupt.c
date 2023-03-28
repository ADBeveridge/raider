#include "corrupt.h"

static uint8_t corrupt_step(const char *filename, const off_t filesize, const char *pattern, struct _corrupt_data *corrupt_data, int loop)
{
    uint8_t ret = 0;

    FILE* fp = fopen(filename,  "r+");
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
                if (g_task_return_error_if_cancelled (corrupt_data->task)) {fclose(fp);return 1;}

                double current = ((double)loop/32.0) + (double)i/times*1.0/32.0;
                if (current - corrupt_data->progress >= .01)
                {
                    corrupt_data->progress = current;
                    g_idle_add_once (raider_file_row_set_progress, corrupt_data);
                }
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

uint8_t corrupt_file(const char *filename, struct _corrupt_data *corrupt_data)
{
    uint8_t ret = 0;
    
    struct stat st;
    if(stat(filename, &st) == 0)
    {
        if (S_ISREG(st.st_mode) != 0)
        {
            off_t filesize = st.st_size;
            const char* steps[] = {"\x77\x77\x77", "\x76\x76\x76",
                                     "\x33\x33\x33", "\x35\x35\x35",
                                     "\x55\x55\x55", "\xAA\xAA\xAA",
                                     "\x92\x49\x24", "\x49\x24\x92",
                                     "\x24\x92\x49", "\x00\x00\x00",
                                     "\x11\x11\x11", "\x01\x01\x01",
                                     "\x22\x22\x22", "\x33\x33\x33",
                                     "\x44\x44\x44", "\x55\x55\x55",
                                     "\x66\x66\x66", "\x77\x77\x77",
                                     "\x88\x88\x88", "\x99\x99\x99",
                                     "\xAA\xAA\xAA", "\xBB\xBB\xBB",
                                     "\xCC\xCC\xCC", "\xDD\xDD\xDD",
                                     "\xEE\xEE\xEE", "\xFF\xFF\xFF",
                                     "\x92\x49\x24", "\x49\x24\x92",
                                     "\x24\x92\x49", "\x6D\xB6\xDB",
                                     "\xB6\xDB\x6D", "\xDB\x6D\xB6"};

            uint8_t i;
            for (i = 0; i < 32; i++)
            {
                if (corrupt_step(filename, filesize, steps[i], corrupt_data, i) != 0)
                {
                    ret = 1;
                    break;
                }
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

    if (remove(filename) != 0)
    {
        ret = 1;
    }

    return ret;
}

