#include "safeallocs.h"

#include <stdlib.h>
#include <stdio.h>

const char *memallocerrorstr = "Memory (re)allocation failed. Application aborted.";

void *malloc_s(size_t size)
{
    void *ret = malloc(size);
    if (!ret) { puts(memallocerrorstr); abort(); }
    return ret;
}

void *realloc_s(void *ptr, size_t size)
{
    void *ret = realloc(ptr, size);
    if (!ret) { puts(memallocerrorstr); abort(); }
    return ret;
}