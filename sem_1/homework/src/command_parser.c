#include "command_parser.h"
#include "string.h"

#include <ctype.h>

#define PIPE_SEPARATOR        "|"
#define ARGUMENT_SEPARATORS   " \t\n"

#define MAX_ARGS     256
#define MAX_COMMANDS 16

static char*    fixBeforeTok(char *tok, char *pipe_pos);
static char*    fixAfterTok (char *pipe_pos);
static CmdError InitCommand (Command *cmd);

CommandLine* InitCommandLine() {
    CommandLine *line = (CommandLine*)calloc(1, sizeof(CommandLine));
    if (line == NULL) {
        return NULL;
    }
    line->cmds = (Command*)calloc(MAX_COMMANDS, sizeof(Command));
    if (line->cmds == NULL) {
        free(line);
        return NULL;
    }
    line->cmd_count = 0;
    return line;
}

void FreeCommandLine(CommandLine *line) {
    if (line == NULL) return;

    for (size_t i = 0; i < line->cmd_count; i++) {
        for (size_t j = 0; j < line->cmds[i].argc; j++) {
            free(line->cmds[i].argv[j]);
        }
        free(line->cmds[i].argv);
    }

    free(line->cmds);
    free(line);
}

static CmdError InitCommand(Command *cmd) {
    assert(cmd);

    cmd->argv = (char**)calloc(MAX_ARGS, sizeof(char*));
    if (cmd->argv == NULL) {
        return ALLOC_ERR;
    }

    cmd->argc = 0;
    return OK;
}

static CmdError AddArgument(Command *cmd, char *arg) {
    if (cmd->argc >= MAX_ARGS - 1) { 
        free(arg); 
        return TOO_MANY_ARGS;
    }
    cmd->argv[cmd->argc++] = arg; 
    return OK;
}

CmdError ParseCommandLine(const char *input, CommandLine *out) {
    assert(input);
    assert(out);

    char* input_copy = strdup(input);
    if (input_copy == NULL) {
        return ALLOC_ERR;
    }

    if (InitCommand(&out->cmds[0]) != OK) {
        free(input_copy);
        return ALLOC_ERR;
    }

    char *save_ptr = NULL;
    for (char *tok = strtok_r(input_copy, ARGUMENT_SEPARATORS, &save_ptr);
         tok != NULL;
         tok = strtok_r(NULL, ARGUMENT_SEPARATORS, &save_ptr))
    {
        Command *cur = &out->cmds[out->cmd_count];
        char *pipe_pos = strchr(tok, '|');

        if (strcmp(tok, PIPE_SEPARATOR) == 0) {
            out->cmd_count++;

            if (out->cmd_count >= MAX_COMMANDS) {
                free(input_copy);
                return TOO_MANY_COMMANDS;
            }

            if (InitCommand(&out->cmds[out->cmd_count]) != OK) {
                free(input_copy);
                return ALLOC_ERR;
            }

            continue; 
        }

        if (pipe_pos != NULL) {
            char *before = fixBeforeTok(tok, pipe_pos);
            if (before != NULL) {
                if (AddArgument(cur, before) != OK) {
                    free(input_copy);
                    return TOO_MANY_ARGS;
                }
            }

            out->cmd_count++;
            if (out->cmd_count >= MAX_COMMANDS) {
                free(input_copy);
                return TOO_MANY_COMMANDS;
            }

            cur = &out->cmds[out->cmd_count];
            if (InitCommand(cur) != OK) {
                free(input_copy);
                return ALLOC_ERR;
            }

            char *after = fixAfterTok(pipe_pos);
            if (after != NULL) {
                if (AddArgument(cur, after) != OK) {
                    free(input_copy);
                    return TOO_MANY_ARGS;
                }
            }

            continue;
        }

        if (AddArgument(cur, strdup(tok)) != OK) {
            free(input_copy);
            return TOO_MANY_ARGS;
        }
    }

    if (out->cmds[out->cmd_count].argc > 0) {
        out->cmd_count++;
    }

    free(input_copy);
    return OK;
}

static char* fixBeforeTok(char *tok, char *pipe_pos) {
    assert(tok);
    assert(pipe_pos);

    size_t len_str = (size_t)(pipe_pos - tok);
    if (len_str == 0) {
        return NULL;
    }

    char *dup = (char*)calloc(len_str + 1, sizeof(char));
    if (dup == NULL) {
        return NULL;
    }

    return strncpy(dup, tok, len_str);
}

static char* fixAfterTok(char *pipe_pos) {
    assert(pipe_pos);

    if (strlen(pipe_pos) == 1) {
        return NULL;
    }
    else {
        char *start = pipe_pos + 1;

        while (isspace(*start)) {
            start++;
        }

        size_t len_str = strlen(start);
        char *dup = (char*)calloc(len_str + 1, sizeof(char));
        if (dup == NULL) {
            return NULL;
        }

        return strncpy(dup, start, len_str);
    }
}

void PrintCommandLineTable(CommandLine *cline) {
    printf("Parsed %zu commands:\n", cline->cmd_count);

    for (size_t i = 0; i < cline->cmd_count; i++) 
    {
        printf("Command %zu (argc=%zu): ", i, cline->cmds[i].argc);
        for (size_t j = 0; j < cline->cmds[i].argc; j++) 
        {
            printf("[%s] ", cline->cmds[i].argv[j]);
        }
        printf("\n"); 
    }
}
