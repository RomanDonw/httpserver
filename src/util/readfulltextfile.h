#ifndef READFULLTEXTFILE_H
#define READFULLTEXTFILE_H

/*
    Returns an allocated block of memory (string) that contains read specified file content.
    Can return NULL (when error occured).
*/
char *readfulltextfile(const char *filepath);

#endif
