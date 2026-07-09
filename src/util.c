#include "util.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdarg.h>

recvallresult recvallwithtimeout(const Socket *socket, void **data, size_t *size, monotime_t singlemaxwaittime, monotime_t fullmaxwaittime)
{
    NError err;
    char *ret = NULL;
    size_t retsz = 0;

    monotime_t lastrecv, currtime, starttime;
    if (!monotime_now(&starttime)) goto errorquit_monotime;
    lastrecv = starttime;

    char buff[512];
    size_t availsz;
    while (true)
    {
        if (!monotime_now(&currtime)) goto errorquit_monotime;
        if (currtime >= lastrecv + singlemaxwaittime || currtime >= starttime + fullmaxwaittime) break;

        if (!socket_isnonblocking(socket))
        {
            if ((err = socket_getreadablebytes(socket, &availsz)) != NError_Success) goto errorquit_socket;
            if (!availsz) continue;
        }

        if ((err = socket_recv(socket, buff, sizeof(buff), &availsz, SOCKET_RECV_NOFLAGS)) != NError_Success)
        {
            if (err == NError_WouldBlock) continue;
            goto errorquit_socket;
        }
        if (!availsz) { err = NError_ConnectionReset; goto errorquit_socket; }

        {
            void *new_ret = realloc(ret, retsz + availsz);
            if (!new_ret) { err = NError_MemoryAllocationFailed; goto errorquit_socket; }
            ret = new_ret;
        }
        memcpy(ret + retsz, buff, availsz);
        retsz += availsz;

        if (!monotime_now(&lastrecv)) goto errorquit_monotime;
    }

    *data = ret;
    *size = retsz;
    return (recvallresult){ .type = RECVALL_NOERROR };

    errorquit_socket:
        if (ret) free(ret);
    return (recvallresult){ .type = RECVALL_NError, .error.generic = err };

    errorquit_monotime:
        if (ret) free(ret);
    return (recvallresult){ .type = RECVALL_OWNERROR, .error.own = RECVALLERROR_MONOTIME };
}

void __logerror(const char *filename, int line, const char *functionname, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    fprintf(stderr, "[file \"%s\" line %i function \"%s\"]: ", filename, line, functionname);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);

    va_end(args);
}

static const char *memallocerrorstr = "memory (re)allocation failed.";

void *malloc_s(size_t size)
{
    void *ret = malloc(size);
    if (!ret) { logerror(memallocerrorstr); abort(); }
    return ret;
}

void *realloc_s(void *ptr, size_t size)
{
    void *ret = realloc(ptr, size);
    if (!ret) { logerror(memallocerrorstr); abort(); }
    return ret;
}

bool fullreadfile(char **str, size_t *size, const char *filepath)
{
    const size_t BUFFER_SIZE = 512;

    char *ret = NULL;
    size_t sz = 0;

    FILE *f = fopen(filepath, "r");
    if (!f) return false;

    char buffer[BUFFER_SIZE];

    size_t readblocks;
    while ((readblocks = fread(buffer, 1, BUFFER_SIZE, f)))
    {
        // allocate memory
        {
            char *new_ret = realloc(ret, sz + readblocks);
            if (!new_ret) goto errorquit;
            ret = new_ret;
        }

        memcpy(ret + sz, buffer, readblocks);

        sz += readblocks;
    }

    fclose(f);
    *str = ret;
    if (size) *size = sz;
    return true;

    errorquit:
        if (ret) free(ret);
        fclose(f);
    return false;
}

bool formatstr(char **str, size_t *size, const char *format, ...)
{
    va_list args1, args2;
    va_start(args1, format);
    va_copy(args2, args1);

    int sz = vsnprintf(NULL, 0, format, args1);
    va_end(args1);
    if (sz++ < 0) { va_end(args2); return false; }

    char *ret = malloc(sz);
    if (!ret) { va_end(args2); return false; }

    if (sz > 1)
    {
        bool failed = vsnprintf(ret, sz, format, args2) <= 0;
        va_end(args2);
        if (failed) { free(ret); return false; }
    }
    else
    { va_end(args2); *ret = '\0'; }

    *str = ret;
    if (size) *size = sz;
    return true;
}

bool isoutofbound(const void *inbuffptr, const void *onbuffptr, size_t buffsize)
{ return inbuffptr - onbuffptr >= buffsize || inbuffptr < onbuffptr; }

bool memfcmp(const void *data1, size_t size1, const void *data2, size_t size2)
{
    if (size1 != size2) return false;
    if (!size1) return true;

    for (size_t i = 0; i < size1; i++) if (((const char *)data1)[i] != ((const char *)data2)[i]) return false;

    return true;
}