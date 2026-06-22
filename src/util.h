#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdlib.h>
#include <libmonotime.h>
#include <libsocket.h>
#include <stdbool.h>

#if defined(_MSC_VER) && (_MSC_VER < 1300)
    #define __func__ __FUNCTION__
#endif

void __logerror(const char *filename, int line, const char *functionname, const char *format, ...);
#define logerror(...) (__logerror(__FILE__, __LINE__, __func__, __VA_ARGS__))

#define monotime_now_s(time_ptr) \
    {\
        if (!monotime_now(time_ptr))\
        { logerror("occured system error on getting monotonic time."); exit(EXIT_FAILURE); }\
    }

void *malloc_s(size_t size); // can`t return NULL.
void *realloc_s(void *ptr, size_t size); // can`t return NULL.


enum recvallresulttype
{
    RECVALL_NOERROR,
    RECVALL_SOCKETERROR,
    RECVALL_OWNERROR
} typedef recvallresulttype;

enum recvallerror
{
    RECVALLERROR_MONOTIME
} typedef recvallerror;

struct recvallresult
{
    recvallresulttype type;
    union
    {
        SocketError socket;
        recvallerror own;
    } error;
} typedef recvallresult;

/*
    Stores pointer to an allocated block of memory with all read data from socket taking into account the timeout.
    can store *data = NULL & *size = 0 if no data read.
    no changes *data & *size on error.
*/

recvallresult recvallwithtimeout(const Socket *socket, void **data, size_t *size, monotime_t singlemaxwaittime, monotime_t fullmaxwaittime);

/*
Returns an allocated block of memory (string) that contains read specified file content.
Can return NULL (when error occured).
*/
bool fullreadfile(char **str, size_t *size, const char *filepath);

/*
    Formats string and passes it through 'str' parameter.
    Parameter 'size' is optional (can be NULL).
    Returns true on success and false on failed.
    Values passes by pointers changed only if function completed successfully.
    Result string allocates in heap and requires 'free' call after usage.
*/
bool formatstr(char **str, size_t *size, const char *format, ...);

bool isoutofbound(const void *inbuffptr, const void *onbuffptr, size_t buffsize);

#endif