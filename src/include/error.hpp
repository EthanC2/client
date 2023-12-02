#pragma once

#include <errno.h>

#define errchk(errcode, msg)     \
    do                           \
    {                            \
        if ((errcode) == -1)     \
        {                        \
            perror(msg);         \
            exit(errno);         \
        }                        \
    }                            \
    while (0)