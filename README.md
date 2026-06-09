uses my [libsocket](https://github.com/RomanDonw/libsocket) for platform-independent sockets API.

on Windows requires existing `getopt`-provider library.

C23 standard required for `timespec_get` function, because i need milli- and possible micro- -second time precision.