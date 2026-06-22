#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>

struct HTTPHeader
{
    const char *name;
    const char *value;
} typedef HTTPHeader;

struct HTTPRequest
{
    char *__raw;

    const char *method;
    const char *url;
    const char *version;

    HTTPHeader *headers;
    size_t headerscount;

    const char *body;
} typedef HTTPRequest;

enum parseHTTPrequesterror
{
    PARSEREQUESTERROR_SUCCESS,

    PARSEREQUESTERROR_NOMEM,
    PARSEREQUESTERROR_INCORRREQ,
} typedef parseHTTPrequesterror_t;

parseHTTPrequesterror_t parseHTTPrequest(HTTPRequest *request, const char *raw, size_t rawsize); // rawsize cam be NULL.
void freeHTTPrequest(const HTTPRequest *request);

#endif