#include <stdio.h>
#include <assert.h>

#include "common.h"
#include "command_parser.h"

#define CMD_BUFFER 1024

char* ReadCmd() 
{
    char *string_cmd = (char*)calloc(CMD_BUFFER, sizeof(char));
    if (string_cmd == NULL) 
    {
        return NULL;
    }

    printf(COLOR_GREEN "rAch-kaplin-shell> " COLOR_RESET);
    if (fgets(string_cmd, CMD_BUFFER, stdin) == NULL) 
    {
        fprintf(stderr, "Error reading string_cmd\n");
        return NULL;
    }

    return string_cmd;
}
