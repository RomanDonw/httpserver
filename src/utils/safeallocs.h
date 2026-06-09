#ifndef SAFEALLOCS_H
#define SAFEALLOCS_H

#include <stddef.h>

void *malloc_s(size_t size); // can`t return NULL.
void *realloc_s(void *ptr, size_t size); // can`t return NULL.

#endif