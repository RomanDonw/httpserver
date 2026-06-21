#include "util.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdarg.h>

recvallresult recvallwithtimeout(const Socket *socket, monotime_t timeout, void **data, size_t *size)
{
    SocketError err;
    char *ret = NULL;
    size_t retsz = 0;

    monotime_t lastrecv, currtime;
    if (!monotime_now(&lastrecv)) goto errorquit_monotime;
    
    char buff[512];
    size_t availsz;
    while (true)
    {
        if (!monotime_now(&currtime)) goto errorquit_monotime;
        if (currtime >= lastrecv + timeout) break;

        if (!socket_isnonblocking(socket))
        {
            if ((err = socket_getreadablebytes(socket, &availsz)) != SocketError_Success) goto errorquit_socket;
            if (!availsz) continue;
        }

        if ((err = socket_recv(socket, buff, sizeof(buff), &availsz, SOCKET_RECV_NOFLAGS)) != SocketError_Success)
        {
            if (err == SocketError_WouldBlock) continue;
            goto errorquit_socket;
        }
        if (!availsz) { err = SocketError_ConnectionReset; goto errorquit_socket; }

        {
            void *new_ret = realloc(ret, retsz + availsz);
            if (!new_ret) { err = SocketError_MemoryAllocationFailed; goto errorquit_socket; }
            ret = new_ret;
        }
        memcpy(ret + retsz, buff, availsz);
        retsz += availsz;

        if (!monotime_now(&lastrecv)) goto errorquit_monotime;
    }

    *data = ret;
    if (size) *size = retsz;
    return (recvallresult){ .type = RECVALL_NOERROR };

    errorquit_socket:
        if (ret) free(ret);
    return (recvallresult){ .type = RECVALL_SOCKETERROR, .error.socket = err };

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

    // expand buffer to add zero string terminator byte to end of buffer.
    {
        char *new_ret = realloc(ret, sz + 1);
        if (!new_ret) goto errorquit;
        ret = new_ret;
    }

    ret[sz++] = '\0';

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
    if (sz <= 0) return false;
    va_end(args1);

    char *ret = malloc(sz);
    if (!ret) return false;

    if (vsnprintf(ret, sz, format, args2) <= 0) { free(ret); return false; }
    va_end(args2);

    *str = ret;
    if (size) *size = sz;
    return true;
}