#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <string.h>

#include "util/readfulltextfile.h"

char *readfile(const char *filename);

int main(int argc, char **argv)
{
    char *configfname = NULL;
    
    {
        int p;
        while ((p = getopt(argc, argv, "c:")) != -1)
        {
            switch (p)
            {
                case 'c':
                    configfname = optarg;
                    break;
            }
        }
    }

    if (!configfname)
    {
        puts("Missed required parameter \"-c\" - config .json file path.");
        return 1;
    }

    char *configcontent = readfulltextfile(configfname);
    if (!configcontent)
    {
        printf("Can't open file \"%s\".\n", configfname);
        return 1;
    }

    puts(configcontent);

    free(configcontent);

    return 0;
}