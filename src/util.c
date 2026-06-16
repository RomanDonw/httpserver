#include "util.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

SocketError recvallwithtimeout(const Socket *socket, monotime_t timeout, void **data, size_t *size)
{
    SocketError err;
    char *ret = NULL;
    size_t retsz = 0;

    monotime_t lastrecv, currtime;
    monotime_now_s(&lastrecv);
    
    char buff[512];
    size_t availsz;
    while (true)
    {
        monotime_now_s(&currtime);
        if (currtime >= lastrecv + timeout) break;

        if (!socket_isnonblocking(socket))
        {
            if ((err = socket_getreadablebytes(socket, &availsz)) != SocketError_Success) goto errorquit;
            if (!availsz) continue;
        }

        if ((err = socket_recv(socket, buff, sizeof(buff), &availsz, SOCKET_RECV_NOFLAGS)) != SocketError_Success)
        {
            if (err == SocketError_WouldBlock) continue;
            goto errorquit;
        }
        if (!availsz) { err = SocketError_ConnectionReset; goto errorquit; }

        {
            void *new_ret = realloc(ret, retsz + availsz);
            if (!new_ret) { err = SocketError_MemoryAllocationFailed; goto errorquit; }
            ret = new_ret;
        }
        memcpy(ret + retsz, buff, availsz);
        retsz += availsz;

        monotime_now_s(&lastrecv);
    }

    *data = ret;
    if (size) *size = retsz;
    return SocketError_Success;

    errorquit:
        if (ret) free(ret);
    return err;
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

char *readfulltextfile(const char *filename)
{
    const size_t BUFFER_SIZE = 512;

    char *ret = NULL;
    size_t size = 0;

    FILE *f = fopen(filename, "r");
    if (!f) return NULL;

    char buffer[BUFFER_SIZE];

    size_t readblocks;
    while (readblocks = fread(buffer, sizeof(buffer), 1, f))
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