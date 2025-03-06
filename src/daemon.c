#include "../include/daemon.h"
#include "../include/company.h"

// Global variables
int running = 1;
int transfer_in_progress = 0;
int msgid;

// Function to daemonize the process
void daemonize(void) {
    pid_t pid = fork();
    
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    umask(0);
    
    pid_t sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }
    
    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

// Signal handler
void signal_handler(int sig) {
    switch(sig) {
        case SIGTERM:
            log_message(LOG_INFO, "Received SIGTERM signal, shutting down");
            running = 0;
            break;
        case SIGUSR1:
            if (!transfer_in_progress) {
                log_message(LOG_INFO, "Received SIGUSR1 signal, starting manual backup/transfer");
                if (lock_directories() == 0) {
                    backup_reporting_dir();
                    transfer_uploads();
                    unlock_directories();
                }
            }
            break;
    }
}

// Setup signal handlers
void setup_signals(void) {
    signal(SIGTERM, signal_handler);
    signal(SIGUSR1, signal_handler);
    signal(SIGHUP, SIG_IGN);  // Ignore SIGHUP
}

// Check for singleton instance
int check_singleton(const char *lockfile) {
    int fd = open(lockfile, O_RDWR | O_CREAT, 0640);
    if (fd < 0) {
        return 0;
    }
    
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    
    if (fcntl(fd, F_SETLK, &fl) < 0) {
        close(fd);
        return 0;
    }
    
    return 1;
}

// Write PID to file
void write_pid_file(const char *pidfile) {
    FILE *f = fopen(pidfile, "w");
    if (f) {
        fprintf(f, "%d\n", getpid());
        fclose(f);
    }
}

// Clean up before exit
void cleanup(void) {
    if (transfer_in_progress) {
        unlock_directories();
    }
    
    cleanup_ipc(msgid);
    unlink(PID_FILE);
}

int main(void) {
    // Check if another instance is running
    if (!check_singleton(LOCK_FILE)) {
        fprintf(stderr, "Another instance is already running\n");
        exit(EXIT_FAILURE);
    }
    
    // Daemonize
    daemonize();
    
    // Open syslog
    openlog("company_daemon", LOG_PID, LOG_DAEMON);
    
    // Write PID to file
    write_pid_file(PID_FILE);
    
    // Create directories if they don't exist
    mkdir(UPLOAD_DIR, 0755);
    mkdir(REPORTING_DIR, 0755);
    mkdir(BACKUP_DIR, 0755);
    mkdir(LOG_DIR, 0755);
    
    // Create department directories
    char path[256];
    sprintf(path, "%s/warehouse", UPLOAD_DIR);
    mkdir(path, 0755);
    sprintf(path, "%s/manufacturing", UPLOAD_DIR);
    mkdir(path, 0755);
    sprintf(path, "%s/sales", UPLOAD_DIR);
    mkdir(path, 0755);
    sprintf(path, "%s/distribution", UPLOAD_DIR);
    mkdir(path, 0755);
    
    // Set up signal handlers
    setup_signals();
    
    // Set up IPC
    msgid = setup_ipc();
    
    log_message(LOG_INFO, "Daemon started");
    
    // Main loop
    while (running) {
        // Get current time
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        
        // Check if it's time for scheduled backup/transfer (1 AM)
        if (tm_info->tm_hour == TRANSFER_TIME_HOUR && 
            tm_info->tm_min == TRANSFER_TIME_MIN && 
            !transfer_in_progress) {
            
            log_message(LOG_INFO, "Starting scheduled backup and transfer");
            
            transfer_in_progress = 1;
            
            if (lock_directories() == 0) {
                backup_reporting_dir();
                transfer_uploads();
                check_missing_uploads();
                unlock_directories();
            }
            
            transfer_in_progress = 0;
        }
        
        // Monitor upload directory for changes
        monitor_uploads();
        
        // Sleep for a minute
        sleep(60);
    }
    
    // Cleanup before exit
    cleanup();
    closelog();
    
    return EXIT_SUCCESS;
}