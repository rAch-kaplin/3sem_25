#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

typedef enum CmdError
{
    OK                  = 0x0000,  
    ALLOC_ERR           = 0x0001,  
    READ_ERR            = 0x0002,  
    TOO_MANY_COMMANDS   = 0x0003,  
    TOO_MANY_ARGS       = 0x0004,  
    SYNTAX_ERR          = 0x0005,
    NULL_PTR            = 0x0006,
} CmdError;

#endif // COMMON_H
