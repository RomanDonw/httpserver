#include <getopt.h>
#include <stdio.h>
#include <libsocket.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    IPv4Address addr = IPV4ADDR_LOOPBACK;
    unsigned short port = 80;
    
    {
        int p;
        while ((p = getopt(argc, argv, "a:p:")) != -1)
        {
            switch (p)
            {
                case 'p':
                    if (sscanf(optarg, "%hd", &port) < 1)
                    {
                        fputs("Error parsing -p parameter value.", stderr);
                        exit(EXIT_FAILURE);
                    }
                    break;
            }
        }
    }

    return 0;
}