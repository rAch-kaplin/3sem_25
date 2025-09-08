#include "command_parser.h"
#include "common.h"
#include "run_cmd.h"

int main()
{
    char *string_cmd = ReadCmd();
    if (string_cmd == NULL) 
    {
        fprintf(stderr, "failed read cmd string\n");
        return 1;
    }

    CommandLine *cline = InitCommandLine();
    CmdError err = ParseCommandLine(string_cmd, cline);
    if (err != OK) 
    {
        fprintf(stderr, "ParseCommandLine failed with error %d\n", err);
        return 1;
    }
    free(string_cmd);
    
    #if 0
    PrintCommandLineTable(cline);
    #endif

    RunCmd(cline);

    FreeCommandLine(cline);
    return 0;
}
