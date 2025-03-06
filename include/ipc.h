// #ifndef IPC_H
// #define IPC_H

// #include "daemon.h"
// #include <sys/ipc.h>
// #include <sys/msg.h>

// // Message Structure For IPC
// struct msg_buffer {
//     long msg_type;
//     char msg_text[100];
// };

// // Message Types
// #define MSG_BACKUP_START 1
// #define MSG_BACKUP_DONE 2
// #define MSG_BACKUP_FAIL 3
// #define MSG_TRANSFER_START 4
// #define MSG_TRANSFER_DONE 5
// #define MSG_TRANSFER_FAIL 6

// // Function Declarations For IPC
// int setup_ipc(void);
// int send_message(long msg_type, const char *message);
// int receive_message(struct msg_buffer *message);
// void cleanup_ipc(void);

// #endif /* IPC_H */