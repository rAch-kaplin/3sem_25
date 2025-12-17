#ifndef COMMON_H
#define COMMON_H

#define SERVER_FIFO_PATH    "tmp/server"
#define CLIENT_PREFIX       "tmp/client_"
#define MAX_FILENAME_LEN    256
#define BUFFER_SIZE         4096
#define MAX_CLIENTS         64


#define CMD_REGISTER "REGISTER"
#define CMD_GET      "GET"
#define CMD_DISCONNECT "DISCONNECT"
#define RESP_ACK     "ACK"
#define RESP_SIZE    "SIZE"

#endif // COMMON_H
