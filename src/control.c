#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#define PID_FILE "/var/run/company_daemon.pid"

void usage(void) {
    printf("Usage: company_control {start|stop|status|backup}\n");
    exit(EXIT_FAILURE);
}

// Start the daemon
void start_daemon(void) {
    // Check if daemon is already running
    FILE *pid_file = fopen(PID_FILE, "r");
    if (pid_file) {
        pid_t pid;
        if (fscanf(pid_file, "%d", &pid) == 1) {
            if (kill(pid, 0) == 0) {
                printf("Daemon is already running with PID %d\n", pid);
                fclose(pid_file);
                exit(EXIT_FAILURE);
            }
        }
        fclose(pid_file);
    }
    
    // Start daemon
    printf("Starting daemon...\n");
    system("/usr/sbin/company_daemon");
    sleep(1);
    
    // Check if daemon started
    pid_file = fopen(PID_FILE, "r");
    if (pid_file) {
        pid_t pid;
        if (fscanf(pid_file, "%d", &pid) == 1) {
            printf("Daemon started with PID %d\n", pid);
        } else {
            printf("Daemon started, but could not read PID\n");
        }
        fclose(pid_file);
    } else {
        printf("Failed to start daemon\n");
        exit(EXIT_FAILURE);
    }
}

// Stop the daemon
void stop_daemon(void) {
    FILE *pid_file = fopen(PID_FILE, "r");
    if (!pid_file) {
        printf("Daemon is not running\n");
        exit(EXIT_FAILURE);
    }
    
    pid_t pid;
    if (fscanf(pid_file, "%d", &pid) != 1) {
        printf("Failed to read PID file\n");
        fclose(pid_file);
        exit(EXIT_FAILURE);
    }
    
    fclose(pid_file);
    
    printf("Stopping daemon with PID %d...\n", pid);
    if (kill(pid, SIGTERM) < 0) {
        printf("Failed to stop daemon: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    printf("Daemon stopped\n");
}

// Check daemon status
void check_status(void) {
    FILE *pid_file = fopen(PID_FILE, "r");
    if (!pid_file) {
        printf("Daemon is not running\n");
        exit(EXIT_FAILURE);
    }
    
    pid_t pid;
    if (fscanf(pid_file, "%d", &pid) != 1) {
        printf("Failed to read PID file\n");
        fclose(pid_file);
        exit(EXIT_FAILURE);
    }
    
    fclose(pid_file);
    
    if (kill(pid, 0) == 0) {
        printf("Daemon is running with PID %d\n", pid);
    } else {
        printf("Daemon is not running (stale PID file)\n");
        exit(EXIT_FAILURE);
    }
}

// Trigger manual backup/transfer
void backup(void) {
    FILE *pid_file = fopen(PID_FILE, "r");
    if (!pid_file) {
        printf("Daemon is not running\n");
        exit(EXIT_FAILURE);
    }
    
    pid_t pid;
    if (fscanf(pid_file, "%d", &pid) != 1) {
        printf("Failed to read PID file\n");
        fclose(pid_file);
        exit(EXIT_FAILURE);
    }
    
    fclose(pid_file);
    
    printf("Triggering backup/transfer...\n");
    if (kill(pid, SIGUSR1) < 0) {
        printf("Failed to send signal: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    printf("Backup/transfer triggered\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage();
    }
    
    if (strcmp(argv[1], "start") == 0) {
        start_daemon();
    } else if (strcmp(argv[1], "stop") == 0) {
        stop_daemon();
    } else if (strcmp(argv[1], "status") == 0) {
        check_status();
    } else if (strcmp(argv[1], "backup") == 0) {
        backup();
    } else {
        usage();
    }
    
    return EXIT_SUCCESS;
}