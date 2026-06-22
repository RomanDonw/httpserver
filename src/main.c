#include <getopt.h>
#include <stdio.h>
#include <libsocket.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <libmonotime.h>

#include "request.h"
#include "util.h"

// herr_... - error handler prefix.
void herr_libsocket(SocketError err) { fprintf(stderr, "libsocket error: %s.\n", socket_strerror(err)); exit(EXIT_FAILURE); }
void herr_parsearg(const char *pname) { fprintf(stderr, "Error parsing %s parameter value.\n", pname); exit(EXIT_FAILURE); }

#define CHECKSOCKERR(err_var, code) { if ((err_var = (code)) != SocketError_Success) herr_libsocket(err_var); }

void sendresp_badrequest(const Socket *socket);
void sendresp_intrserverr(const Socket *socket);

// 'volatile' need to prevent 'ignoring' on optimization.
volatile bool working = true;

void handlesig(int sig)
{
    switch (sig)
    {
        case SIGTERM:
        case SIGINT:
            putchar('\n');
            working = false;
            break;
    }
}

int main(int argc, char **argv)
{
    signal(SIGTERM, handlesig);
    signal(SIGINT, handlesig);

    if (!monotime_now(NULL)) { fputs("This platform doesn't support monotonic time.", stderr); return 1; }

    // =============================================================================

    SocketError err;
    IPv4Address addr = IPV4ADDR_LOOPBACK;
    char *addrstr = "127.0.0.1";
    unsigned short port = 80;

    if ((err = libsocket_startup(NULL)) != SocketError_Success) herr_libsocket(err);
    
    {
        int p;
        while ((p = getopt(argc, argv, "a:p:")) != -1)
        {
            switch (p)
            {
                case 'a':
                    if ((err = socket_parseaddr(&addr, SocketAddressFamily_IPv4, optarg)) != SocketError_Success)
                    {
                        if (err == SocketError_ParsingAddressFailed) herr_parsearg("-a");
                        else herr_libsocket(err);
                    }
                    addrstr = optarg;
                    break;

                case 'p':
                    if (sscanf(optarg, "%hu", &port) < 1) herr_parsearg("-p");
                    break;
            }
        }
    }

    char *page = NULL;
    size_t pagesize = 0;
    if (!fullreadfile(&page, &pagesize, "res/page.html")) { puts("Error reading \"res/page.html\" file."); return 1; }

    SocketIPv4Address saddr;
    CHECKSOCKERR(err, socket_packsockaddr(&saddr, SocketAddressFamily_IPv4, &addr, port));

    Socket *serv;
    CHECKSOCKERR(err, socket_open(&serv, SocketAddressFamily_IPv4, SocketType_Stream, SocketProtocol_TCP));

    CHECKSOCKERR(err, socket_setnonblocking(serv, true));

    CHECKSOCKERR(err, socket_bind(serv, &saddr, sizeof(saddr)));
    CHECKSOCKERR(err, socket_listen(serv, 8));

    printf("Started HTTP 1.0 server at %s:%hu.\n\n", addrstr, port);

    // =============================================================================

    char ip4str[IPV4ADDRSTRSIZE];
    Socket *cl;
    socklen_t saddrsz;
    char *data = NULL;
    size_t datasize = 0;
    recvallresult recvres;
    parseHTTPrequesterror_t perr;
    size_t accepterrorscounter;

    while (working)
    {
        // accept connection & store client socket address.
        saddrsz = sizeof(saddr);
        if ((err = socket_accept(&cl, serv, &saddr, &saddrsz)) != SocketError_Success)
        {
            if (err == SocketError_WouldBlock) continue;
            printf("Accepting connection error: %s.\n", socket_strerror(err));

            if (accepterrorscounter++ < 5) continue;
            puts("Too many errors raised at the same moment. Shutting down the server.");
            break;
        }
        accepterrorscounter = 0;
        if (saddrsz != sizeof(saddr))
        {
            puts("Internal size mismatch.");
            sendresp_intrserverr(cl);
            goto closeconn;
        }

        // unpack IP and port from SocketAddress structure.
        if ((err = socket_unpacksockaddr(&saddr, SocketAddressFamily_IPv4, &addr, &port)) != SocketError_Success)
        {
            printf("Error unpacking socket address: %s.\n", socket_strerror(err));
            sendresp_intrserverr(cl);
            goto closeconn;
        }

        // output client address to console.
        if ((err = socket_addrtostr(&addr, SocketAddressFamily_IPv4, ip4str, sizeof(ip4str))) != SocketError_Success)
        {
            printf("Error converting IPv4 binary representation to string equivalent: %s.\n", socket_strerror(err));
            sendresp_intrserverr(cl);
            goto closeconn;
        }

        printf("Accepted client %s:%hu.\n", ip4str, port);

        // =============================================================================

        recvres = recvallwithtimeout(cl, (void **)&data, &datasize, 50 * MONOTIME_MILLISECOND, 10 * MONOTIME_SECOND);
        if (recvres.type != RECVALL_NOERROR)
        {
            switch (recvres.type)
            {
                case RECVALL_SOCKETERROR:
                    printf("Occured socket-related error while reading request: %s.", socket_strerror(recvres.error.socket));
                    break;

                case RECVALL_OWNERROR:
                    switch (recvres.error.own)
                    {
                        case RECVALLERROR_MONOTIME:
                            puts("Occured error related with getting monotonic time while reading request.");
                            break;

                        default:
                            puts("Occured unknown error while reading request.");
                    }
                    break;

                default:
                    puts("Occured error with unknown type while reading request.");
            }
            sendresp_intrserverr(cl);
            goto closeconn;
        }
        if (!data) { puts("No request data available."); sendresp_badrequest(cl); goto closeconn; }

        HTTPRequest req;
        perr = parseHTTPrequest(&req, data, datasize);
        free(data);
        if (perr != PARSEREQUESTERROR_SUCCESS)
        {
            switch (perr)
            {
                case PARSEREQUESTERROR_INCORRREQ:
                    puts("Bad request.");
                    sendresp_badrequest(cl);
                    break;

                case PARSEREQUESTERROR_NOMEM:
                    puts("Out of memory while parsing request.");
                    sendresp_intrserverr(cl);
                    break;

                default:
                    puts("Unknown error while parsing request.");
                    sendresp_intrserverr(cl);
            }
            goto closeconn;
        }

        // =============================================================================

        //printf("HTTP request info:\n -  Method: %s.\n -  URL: \"%s\".\n -  Version: %s.\n", req.method, req.url, req.version);

        printf("HTTP headers count: %zu.\nHTTP headers:\n", req.headerscount);
        for (size_t i = 0; i < req.headerscount; i++) printf("    [%zu]: \"%s\" = \"%s\".\n", i, req.headers[i].name, req.headers[i].value);

        printf("HTTP request body size: %zu.\n", req.bodysize);

        if (!memfcmp(req.version, req.versionsize, "HTTP/1.0", sizeof("HTTP/1.0")) && !memfcmp(req.version, req.versionsize, "HTTP/1.1", sizeof("HTTP/1.1")))
        {
            puts("Request HTTP version not supported.");
            const char resp[] = "HTTP/1.0 505 HTTP Version Not Supported\r\n\r\n";
            socket_send(cl, resp, sizeof(resp) - 1, NULL, SOCKET_SEND_NOFLAGS);
            goto finishconn;
        }

        if (!memfcmp(req.method, req.methodsize, "GET", sizeof("GET")) && !memfcmp(req.method, req.methodsize, "HEAD", sizeof("HEAD")))
        {
            puts("Request method not implemented.");
            const char resp[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
            socket_send(cl, resp, sizeof(resp) - 1, NULL, SOCKET_SEND_NOFLAGS);
            goto finishconn;
        }

        {
            char *response = NULL;
            size_t responsesz = 0;

            if (!formatstr(&response, &responsesz, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", pagesize - 1))
            { puts("Error formatting response."); sendresp_intrserverr(cl); goto finishconn; }

            if (!memfcmp(req.method, req.methodsize, "HEAD", sizeof("HEAD")))
            {
                {
                    char *new_response = realloc(response, responsesz + pagesize);
                    if (!new_response) { puts("Not enough memory to complete building response."); sendresp_intrserverr(cl); goto finishconn; }
                    response = new_response;
                }

                memcpy(response + responsesz, page, pagesize);
                responsesz += pagesize;
            }

            socket_send(cl, response, responsesz, NULL, SOCKET_SEND_NOFLAGS);

            free(response);
        }

        // ============================================================================

        finishconn:
            freeHTTPrequest(&req);
            
        closeconn:
            CHECKSOCKERR(err, socket_close(cl));
            printf("Closed connection with %s:%hu.\n\n", ip4str, port);
    }

    // =============================================================================

    free(page);

    CHECKSOCKERR(err, socket_close(serv));
    puts("\nHTTP 1.0 server stopped successfully.");

    CHECKSOCKERR(err, libsocket_cleanup());

    return 0;
}

void sendresp_badrequest(const Socket *socket)
{
    static const char response[] = "HTTP/1.0 400 Bad Request\r\n\r\n";
    socket_send(socket, response, sizeof(response) - 1, NULL, SOCKET_SEND_NOFLAGS);
}

void sendresp_intrserverr(const Socket *socket)
{
    static const char response[] = "HTTP/1.0 500 Internal Server Error\r\n\r\n";
    socket_send(socket, response, sizeof(response) - 1, NULL, SOCKET_SEND_NOFLAGS);
}