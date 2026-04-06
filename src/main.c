#include <getopt.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    {
        int p;
        while ((p = getopt(argc, argv, "c:")) != -1)
        {
            switch (p)
            {
                case 'c':
                    puts(optarg);
                    break;
            }
        }
    }

    return 0;
}