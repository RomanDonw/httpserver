#include "readfulltextfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *readfulltextfile(const char *filename)
{
    const size_t BUFFER_SIZE = 512;

    char *ret = NULL;
    size_t size = 0;

    FILE *f = fopen(filename, "r");
    if (!f) return NULL;

    char buffer[BUFFER_SIZE];

    size_t readblocks;
    while (readblocks = fread(buffer, sizeof(char), BUFFER_SIZE, f))
    {
        // allocate memory
        {
            char *new_ret = realloc(ret, size + readblocks * sizeof(char));
            if (!new_ret) goto errorquit;
            ret = new_ret;
        }

        memcpy(ret + size, buffer, readblocks * sizeof(char));

        size += readblocks * sizeof(char);
    }

    // expand buffer to add zero string terminator byte to end of buffer.
    {
        char *new_ret = realloc(ret, size + sizeof(char));
        if (!new_ret) goto errorquit;
        ret = new_ret;
    }

    ret[size] = '\0';

    fclose(f);
    return ret;

    errorquit:
        if (ret) free(ret);
        fclose(f);
    return NULL;
}
