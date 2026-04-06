#include "readfulltextfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

char *readfulltextfile(const char *filename)
{
    const size_t BUFFER_SIZE = 512;

    char *ret = NULL;
    size_t size = 0;

    FILE *f = fopen(filename, "r");
    if (!f) return NULL;

    char buffer[BUFFER_SIZE];

    while (true)
    {
        size_t readblocks = fread(buffer, sizeof(char), BUFFER_SIZE, f);
        if (!readblocks) break;

        {
            char *new_ret = realloc(ret, size + readblocks * sizeof(char));
            if (!new_ret)
            {
                if (ret) free(ret);
                fclose(f);
                return NULL;
            }
            ret = new_ret;
        }

        memcpy(ret + size, buffer, readblocks * sizeof(char));

        size += readblocks * sizeof(char);
    }

    {
        char *new_ret = realloc(ret, size + 1);
        if (!new_ret)
        {
            if (ret) free(ret);
            fclose(f);
            return NULL;
        }
        ret = new_ret;
    }

    ret[size] = '\0';

    fclose(f);
    return ret;
}