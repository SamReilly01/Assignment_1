#include "../include/company.h"
#include <sys/ipc.h>
#include <sys/msg.h>

// Set up IPC message queue
int setup_ipc(void) {
    // Create key for message queue
    key_t key = ftok("/var/company", 'A');
    if (key == -1) {
        log_message(LOG_ERR, "Failed to create IPC key: %s", strerror(errno));
        return -1;
    }
    
    // Create message queue
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        log_message(LOG_ERR, "Failed to create message queue: %s", strerror(errno));
        return -1;
    }
    
    log_message(LOG_INFO, "IPC message queue created with ID %d", msgid);
    return msgid;
}

// Send a message to the queue
int send_message(int msgid, long type, const char *msg) {
    struct msg_buffer message;
    message.msg_type = type;
    strncpy(message.msg_text, msg, sizeof(message.msg_text) - 1);
    message.msg_text[sizeof(message.msg_text) - 1] = '\0';
    
    if (msgsnd(msgid, &message, sizeof(message.msg_text), IPC_NOWAIT) == -1) {
        log_message(LOG_ERR, "Failed to send message: %s", strerror(errno));
        return -1;
    }
    
    return 0;
}

// Clean up IPC resources
void cleanup_ipc(int msgid) {
    if (msgid != -1) {
        msgctl(msgid, IPC_RMID, NULL);
        log_message(LOG_INFO, "IPC message queue removed");
    }
}