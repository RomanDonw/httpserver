#include <getopt.h>
#include <stdio.h>
#include <libsocket.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <libmonotime.h>

#include "util.h"

// herr_... - error handler prefix.
void herr_libsocket(SocketError err) { fprintf(stderr, "libsocket error: %s.\n", socket_strerror(err)); exit(EXIT_FAILURE); }
void herr_parsearg(const char *pname) { fprintf(stderr, "Error parsing %s parameter value.\n", pname); exit(EXIT_FAILURE); }

#define CHECKSOCKERR(err_var, code) { if ((err_var = (code)) != SocketError_Success) herr_libsocket(err_var); }

void recvallwithtimeout(const Socket *socket, monotime_t timeout, char **readdata, size_t *readbytes);

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

    const char http400msg[] = "Bad request";
    const char http505msg[] = "Method not allowed";

    SocketError err;
    IPv4Address addr = IPV4ADDR_LOOPBACK;
    char *addrstr = "127.0.0.1";
    unsigned short port = 80;

    if ((err = libsocket_startup(NULL, NULL)) != SocketError_Success) herr_libsocket(err);
    
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

    SocketIPv4Address saddr;
    CHECKSOCKERR(err, socket_packsockaddr(&saddr, SocketAddressFamily_IPv4, &addr, port));

    Socket *serv;
    CHECKSOCKERR(err, socket_open(&serv, SocketAddressFamily_IPv4, SocketType_Stream, SocketProtocol_TCP));

    CHECKSOCKERR(err, socket_setnonblocking(serv, true));

    CHECKSOCKERR(err, socket_bind(serv, &saddr, sizeof(saddr)));
    CHECKSOCKERR(err, socket_listen(serv, 8));

    printf("Started HTTP 1.0 server at %s:%hu.\n", addrstr, port);

    // =============================================================================

    char ip4str[IPV4ADDRSTRSIZE];
    Socket *cl;
    socklen_t saddrsz;
    size_t sz;
    char *data = NULL;
    while (working)
    {
        // accept connection & store client socket address.
        saddrsz = sizeof(saddr);
        if ((err = socket_accept(&cl, serv, &saddr, &saddrsz)) != SocketError_Success)
        {
            if (err == SocketError_WouldBlock) continue;
            herr_libsocket(err);
        }
        if (saddrsz != sizeof(saddr)) { puts("Internal size mismatch."); return 1; }

        // unpack IP and port from SocketAddress structure.
        CHECKSOCKERR(err, socket_unpacksockaddr(&saddr, SocketAddressFamily_IPv4, &addr, &port));

        // output client address to console.
        CHECKSOCKERR(err, socket_addrtostr(&addr, SocketAddressFamily_IPv4, ip4str, sizeof(ip4str)));
        printf("Accepted client %s:%hu.\n", ip4str, port);

        // =============================================================================

        recvallwithtimeout(cl, 50 * MONOTIME_MILLISECOND, &data, &sz);

        puts(data);

        #if 0

        char *methodstr = NULL;
        size_t methodlen = 0;
        
        char *_tempptr = strchr(data, ' ');
        if (_tempptr)
        {
            methodlen = _tempptr - data;
            //printf("=== METHOD LEN: %zu\n", methodlen);

            methodstr = malloc_s(methodlen + 1);
            if (methodlen) memcpy(methodstr, data, methodlen);
            methodstr[methodlen] = '\0';
        }
        else { sendHTTPshortresp(cl, 400, http400msg); goto closecl; }

        //printf("=== METHOD: \"%s\"\n", methodstr);

        if (strcmp(methodstr, "GET")) { sendHTTPshortresp(cl, 505, http505msg); goto closecl; }

        char *urlstr = NULL;
        size_t urllen = 0;
        

        char *tmp = strchr(tmp + sizeof(char) * 1, ' ');
        if (tmp)
        {
            methodlen = tmp - data;
            //printf("=== METHOD LEN: %zu\n", methodlen);

            methodstr = malloc_s(methodlen + 1);
            if (methodlen) memcpy(methodstr, data, methodlen);
            methodstr[methodlen] = '\0';
        }
        else { sendHTTPshortresp(cl, 400, http400msg); goto closecl; }

        free(methodstr);
        #endif

        {
            const char page[] = "<h1>Example page</h1><hr>This is an example web page.<br><br>It works!";
            size_t pagelen = sizeof(page) - 1;

            #define FORMATSTR(out_str, size) (snprintf(out_str, size, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n%s\r\n", pagelen, page))

            int responsesz = FORMATSTR(NULL, 0);
            if (responsesz <= 0) { puts("snprintf formatting error."); goto closeconn; }
            
            char *response = malloc_s(responsesz);
            FORMATSTR(response, responsesz);

            puts(response);
            socket_send(cl, response, responsesz, NULL, SOCKET_SEND_NOFLAGS);

            free(response);

            #undef FORMATSTR
        }

        closeconn:
        free(data);

        // =============================================================================

        CHECKSOCKERR(err, socket_close(cl));
        printf("Closed connection with %s:%hu.\n", ip4str, port);
    }

    // =============================================================================

    CHECKSOCKERR(err, socket_close(serv));
    puts("HTTP 1.0 server stopped successfully.");

    CHECKSOCKERR(err, libsocket_cleanup());

    return 0;
}

void recvallwithtimeout(const Socket *socket, monotime_t timeout, char **readdata, size_t *readbytes)
{
    SocketError err;
    char *ret = NULL;
    size_t size = 0;
    char buff[512];

    monotime_t lastrecv, currtime;
    monotime_now_s(&lastrecv);
    
    size_t sz;
    while (true)
    {
        monotime_now_s(&currtime);
        if (currtime >= lastrecv + timeout) break;

        /*
        if ((err = socket_getreadablebytes(socket, &sz)) != SocketError_Success)
        {
            if (err == SocketError_WouldBlock) continue;
            herr_libsocket(err);
        }
        if (!sz) continue;
        */

        if ((err = socket_recv(socket, buff, sizeof(buff), &sz, SOCKET_RECV_NOFLAGS)) != SocketError_Success)
        {
            if (err == SocketError_WouldBlock) continue;
            herr_libsocket(err);
        }
        if (!sz) continue;

        ret = realloc_s(ret, size + sz);
        memcpy(ret + sizeof(char) * size, buff, sz);
        size += sz;

        monotime_now_s(&lastrecv);
    }

    ret = realloc_s(ret, size + 1);
    ret[size] = '\0';

    *readdata = ret;
    if (readbytes) *readbytes = size + 1;
}
