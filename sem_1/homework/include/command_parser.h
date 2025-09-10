#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include "common.h"

typedef struct {
    char    **argv;
    size_t  argc;
} Command;

typedef struct {
    Command  *cmds;
    size_t   cmd_count;
} CommandLine;

CommandLine* InitCommandLine();
CmdError     ParseCommandLine(const char *input, CommandLine *out);
void         FreeCommandLine(CommandLine *line);
char*        ReadCmd();
void         PrintCommandLineTable(CommandLine *cline);

#endif //COMMAND_PARSER_H
