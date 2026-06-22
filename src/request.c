#include "request.h"

#include <stdlib.h>
#include <string.h>

parseHTTPrequesterror_t parseHTTPrequest(HTTPRequest *request, const char *raw, size_t rawsize)
{
    HTTPRequest ret = {0};

    if (!rawsize) rawsize = strlen(raw) + 1;
    ret.__raw = malloc(rawsize);
    if (!ret.__raw) return PARSEREQUESTERROR_NOMEM;
    memcpy(ret.__raw, raw, rawsize);

    // =====================================================

    char *tmp = strchr(ret.__raw, ' ');
    if (!tmp) goto badrequest;
    *tmp = '\0';
    ret.method = ret.__raw;
    
    if (!(*(++tmp))) goto badrequest;

    ret.url = tmp;
    tmp = strchr(tmp, ' ');
    if (!tmp) goto badrequest;
    *tmp = '\0';
    
    if (!(*(++tmp))) goto badrequest;

    ret.version = tmp;
    tmp = strchr(tmp, '\r');
    if (!tmp) goto badrequest;
    *tmp = '\0';

    if (*(++tmp) != '\n') goto badrequest;
    if (*(++tmp) == '\0') goto badrequest;

    // parsing headers.

    if (*tmp != '\r')
    {
        const char *hname;
        const char *hvalue;
        while (*tmp != '\r')
        {
            hname = tmp;
            tmp = strchr(tmp, ':');
            if (!tmp) goto badrequest;
            *tmp = '\0';

            if (*(++tmp) != ' ') goto badrequest;
            if (*(++tmp) == '\0') goto badrequest;

            hvalue = tmp;
            tmp = strchr(tmp, '\r');
            if (!tmp) goto badrequest;
            *tmp = '\0';

            if (*(++tmp) != '\n') goto badrequest;
            if (*(++tmp) == '\0') goto badrequest;

            {
                HTTPHeader *new_headers = realloc(ret.headers, (ret.headerscount + 1) * sizeof(HTTPHeader));
                if (!new_headers) goto nomemforheaders;
                ret.headers = new_headers;
            }

            ret.headers[ret.headerscount++] = (HTTPHeader){ .name = hname, .value = hvalue };
        }
    }

    if (*(++tmp) != '\n') goto badrequest;

    ret.body = ++tmp;

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