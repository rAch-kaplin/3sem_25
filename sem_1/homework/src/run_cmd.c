#include "common.h"
#include "run_cmd.h"

#include <unistd.h>
#include <sys/wait.h>

void RunCmd(CommandLine *cline) {
    assert(cline);

    int   pipeFd[2]       = {-1, -1};
    pid_t pid             = -1;
    int   prev_pipe_read  = -1;

    for (size_t i = 0; i < cline->cmd_count; i++) {
        if (pipe(pipeFd) < 0) {
            exit(1);
        }

        pid = fork();

        if (pid == -1) {
            fprintf(stderr, "failed to create process\n");
            return;
        }
        else if (pid == 0) {
            if (prev_pipe_read != -1) {
                dup2(prev_pipe_read, STDIN_FILENO);  
                close(prev_pipe_read);
            }

            if (i < cline->cmd_count - 1) {
                dup2(pipeFd[1], STDOUT_FILENO);      
                close(pipeFd[0]);                    
                close(pipeFd[1]);                   
            }

            execvp(cline->cmds[i].argv[0], cline->cmds[i].argv);
            exit(1);
        }
        else {
            if (prev_pipe_read != -1) {
                close(prev_pipe_read);
            }

             if (i < cline->cmd_count - 1) {
                close(pipeFd[1]);           
                prev_pipe_read = pipeFd[0]; 
            }
        }
    }

    int status = 0;
    for (size_t i = 0; i < cline->cmd_count; i++) {
        waitpid(-1, &status, 0); 
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("Command %zu ('%s') exited with code %d\n", i, cline->cmds[i].argv[0], exit_code);
        }
    }
}
