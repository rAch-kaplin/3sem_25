#ifndef IPC_H
#define IPC_H

#include "common.h"
#include "daemon.h"

#define MAX_CMD_LEN 256
#define MAX_RESPONSE_LEN 8192

typedef enum {
    CMD_SHOW_DIFF   = 0,    /* Show diff for all changed files */
    CMD_SHOW_K_DIFFS= 1,    /* Show K most recent diffs        */
    CMD_SET_PID     = 2,    /* Set monitored PID               */
    CMD_SET_PERIOD  = 3,    /* Set backup/sample period        */
    CMD_STATUS      = 4,    /* Show daemon status              */
    CMD_RESTORE     = 5,    /* show restored file              */
    CMD_UNKNOWN     = 6     /* Unrecognized or invalid command */
} CommandType;


typedef struct {
    CommandType type;
    int         arg;  // for k diffs, new pid, or period
    char        filename[MAX_PATH_LEN];
} Command;

int init_ipc(void);
void cleanup_ipc(void);
int read_command(Command *cmd);
int process_command(Command *cmd, MonitorState *state, char *response, size_t response_len);
int send_response(const char *response);

#endif //IPC_H


