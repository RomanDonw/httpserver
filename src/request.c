#include "request.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "util.h"

parseHTTPrequesterror_t parseHTTPrequest(HTTPRequest *request, const char *raw, size_t rawsize)
{
    if (!rawsize) return PARSEREQUESTERROR_INVALARG;

    HTTPRequest ret = {0};

    ret.__raw = malloc(rawsize);
    if (!ret.__raw) return PARSEREQUESTERROR_NOMEM;
    memcpy(ret.__raw, raw, rawsize);

    // =====================================================

    ret.method = ret.__raw;
    char *tmp = memchr(ret.__raw, ' ', rawsize);
    if (!tmp) goto badrequest;
    *tmp = '\0';
    ret.methodsize = tmp - ret.method + 1;
    
    if (isoutofbound(++tmp, ret.__raw, rawsize)) goto badrequest;

    ret.url = tmp;
    tmp = memchr(tmp, ' ', rawsize - (tmp - ret.__raw));
    if (!tmp) goto badrequest;
    *tmp = '\0';
    ret.urlsize = tmp - ret.url + 1;
    
    if (isoutofbound(++tmp, ret.__raw, rawsize)) goto badrequest;

    ret.version = tmp;
    tmp = memchr(tmp, '\r', rawsize - (tmp - ret.__raw));
    if (!tmp) goto badrequest;
    *tmp = '\0';
    ret.versionsize = tmp - ret.version + 1;

    if (isoutofbound(++tmp, ret.__raw, rawsize) || *tmp != '\n') goto badrequest;
    if (isoutofbound(++tmp, ret.__raw, rawsize)) goto badrequest;

    // parsing headers.

    if (*tmp != '\r')
    {
        const char *hname;
        const char *hvalue;
        size_t hnamesize;
        size_t hvaluesize;
        while (*tmp != '\r')
        {
            hname = tmp;
            tmp = memchr(tmp, ':', rawsize - (tmp - ret.__raw));
            if (!tmp) goto badrequest;
            *tmp = '\0';
            hnamesize = tmp - hname + 1;

            if (isoutofbound(++tmp, ret.__raw, rawsize) || *tmp != ' ') goto badrequest;
            if (isoutofbound(++tmp, ret.__raw, rawsize)) goto badrequest;

            hvalue = tmp;
            tmp = memchr(tmp, '\r', rawsize - (tmp - ret.__raw));
            if (!tmp) goto badrequest;
            *tmp = '\0';
            hvaluesize = tmp - hvalue + 1;

            if (isoutofbound(++tmp, ret.__raw, rawsize) || *tmp != '\n') goto badrequest;
            if (isoutofbound(++tmp, ret.__raw, rawsize)) goto badrequest;

            {
                HTTPHeader *new_headers = realloc(ret.headers, (ret.headerscount + 1) * sizeof(HTTPHeader));
                if (!new_headers) goto nomemforheaders;
                ret.headers = new_headers;
            }

            ret.headers[ret.headerscount++] = (HTTPHeader)
            {
                .name = hname,
                .namesize = hnamesize,
                .value = hvalue,
                .valuesize = hvaluesize
            };
        }
    }

    if (isoutofbound(++tmp, ret.__raw, rawsize) || *tmp != '\n') goto badrequest;

    if (!isoutofbound(++tmp, ret.__raw, rawsize))
    {
        ret.bodysize = rawsize - (tmp - ret.__raw);
        ret.body = tmp;
    }

    *request = ret;
    return PARSEREQUESTERROR_SUCCESS;

    // =====================================================

    {
        parseHTTPrequesterror_t err;

        badrequest:
            err = PARSEREQUESTERROR_INCORRREQ;
        goto __handleerr;

        nomemforheaders:
            err = PARSEREQUESTERROR_NOMEM;
        goto __handleerr;

        // =========================

        __handleerr:
            if (ret.headers) free(ret.headers);
            free(ret.__raw);
        return err;
    }
}

void freeHTTPrequest(const HTTPRequest *request)
{
    if (!request) return;

    free(request->headers);
    free(request->__raw);
}