#include "../include/company.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdarg.h>
#include <time.h>
#include <utime.h>

// Log a message to both syslog and log file
void log_message(int priority, const char *format, ...) {
    va_list args;
    char message[1024];
    
    // Format message
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // Log to syslog
    syslog(priority, "%s", message);
    
    // Log to file
    if (priority == LOG_ERR) {
        FILE *log_file = fopen(ERROR_LOG, "a");
        if (log_file) {
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
            
            fprintf(log_file, "[%s] %s\n", timestamp, message);
            fclose(log_file);
        }
    }
}

// Lock directories for backup/transfer
int lock_directories(void) {
    log_message(LOG_INFO, "Locking directories");
    
    // Change permissions to read-only
    if (chmod(UPLOAD_DIR, 0555) < 0) {
        log_message(LOG_ERR, "Failed to lock upload directory: %s", strerror(errno));
        return -1;
    }
    
    if (chmod(REPORTING_DIR, 0555) < 0) {
        log_message(LOG_ERR, "Failed to lock reporting directory: %s", strerror(errno));
        chmod(UPLOAD_DIR, 0755);  // Reset upload directory
        return -1;
    }
    
    return 0;
}

// Unlock directories after backup/transfer
int unlock_directories(void) {
    log_message(LOG_INFO, "Unlocking directories");
    
    // Reset permissions
    if (chmod(UPLOAD_DIR, 0755) < 0) {
        log_message(LOG_ERR, "Failed to unlock upload directory: %s", strerror(errno));
        return -1;
    }
    
    if (chmod(REPORTING_DIR, 0755) < 0) {
        log_message(LOG_ERR, "Failed to unlock reporting directory: %s", strerror(errno));
        return -1;
    }
    
    return 0;
}

// Backup reporting directory
int backup_reporting_dir(void) {
    log_message(LOG_INFO, "Starting backup of reporting directory");
    
    // Create timestamped backup directory
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char backup_dir[256];
    strftime(backup_dir, sizeof(backup_dir), "%s/backup_%Y%m%d_%H%M%S", tm_info);
    
    // Create directory
    sprintf(backup_dir, "%s/backup_%d", BACKUP_DIR, (int)now);
    if (mkdir(backup_dir, 0755) < 0) {
        log_message(LOG_ERR, "Failed to create backup directory: %s", strerror(errno));
        return -1;
    }
    
    // Copy files from reporting directory to backup
    DIR *dir = opendir(REPORTING_DIR);
    if (!dir) {
        log_message(LOG_ERR, "Failed to open reporting directory: %s", strerror(errno));
        return -1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char src_path[512], dst_path[512];
        sprintf(src_path, "%s/%s", REPORTING_DIR, entry->d_name);
        sprintf(dst_path, "%s/%s", backup_dir, entry->d_name);
        
        // Check if it's a regular file
        struct stat st;
        if (stat(src_path, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }
        
        // Copy file
        int src_fd = open(src_path, O_RDONLY);
        if (src_fd < 0) {
            log_message(LOG_ERR, "Failed to open source file %s: %s", src_path, strerror(errno));
            continue;
        }
        
        int dst_fd = open(dst_path, O_WRONLY | O_CREAT, 0644);
        if (dst_fd < 0) {
            log_message(LOG_ERR, "Failed to create destination file %s: %s", dst_path, strerror(errno));
            close(src_fd);
            continue;
        }
        
        // Copy data
        char buffer[4096];
        ssize_t bytes_read;
        while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
            write(dst_fd, buffer, bytes_read);
        }
        
        close(src_fd);
        close(dst_fd);
        
        log_message(LOG_INFO, "Backed up %s", entry->d_name);
    }
    
    closedir(dir);
    log_message(LOG_INFO, "Backup completed to %s", backup_dir);
    
    return 0;
}

// Transfer files from upload directory to reporting directory
int transfer_uploads(void) {
    log_message(LOG_INFO, "Starting transfer of uploads");
    
    const char *departments[] = {"warehouse", "manufacturing", "sales", "distribution", NULL};
    
    for (int i = 0; departments[i] != NULL; i++) {
        char dept_dir[256];
        sprintf(dept_dir, "%s/%s", UPLOAD_DIR, departments[i]);
        
        DIR *dir = opendir(dept_dir);
        if (!dir) {
            log_message(LOG_ERR, "Failed to open department directory %s: %s", dept_dir, strerror(errno));
            continue;
        }
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            // Only process XML files
            if (strstr(entry->d_name, ".xml") == NULL) {
                continue;
            }
            
            char src_path[512], dst_path[512];
            sprintf(src_path, "%s/%s", dept_dir, entry->d_name);
            sprintf(dst_path, "%s/%s", REPORTING_DIR, entry->d_name);
            
            // Check if it's a regular file
            struct stat st;
            if (stat(src_path, &st) != 0 || !S_ISREG(st.st_mode)) {
                continue;
            }
            
            // Copy file to reporting directory
            int src_fd = open(src_path, O_RDONLY);
            if (src_fd < 0) {
                log_message(LOG_ERR, "Failed to open source file %s: %s", src_path, strerror(errno));
                continue;
            }
            
            int dst_fd = open(dst_path, O_WRONLY | O_CREAT, 0644);
            if (dst_fd < 0) {
                log_message(LOG_ERR, "Failed to create destination file %s: %s", dst_path, strerror(errno));
                close(src_fd);
                continue;
            }
            
            // Copy data
            char buffer[4096];
            ssize_t bytes_read;
            while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
                write(dst_fd, buffer, bytes_read);
            }
            
            close(src_fd);
            close(dst_fd);
            
            // Preserve ownership and timestamp
            chown(dst_path, st.st_uid, st.st_gid);
            
            struct utimbuf utb;
            utb.actime = st.st_atime;
            utb.modtime = st.st_mtime;
            utime(dst_path, &utb);
            
            // Log transfer
            log_message(LOG_INFO, "Transferred %s from %s to reporting directory", entry->d_name, departments[i]);
            
            // Remove source file
            unlink(src_path);
        }
        
        closedir(dir);
    }
    
    log_message(LOG_INFO, "Transfer completed");
    return 0;
}

// Check for missing uploads
int check_missing_uploads(void) {
    log_message(LOG_INFO, "Checking for missing uploads");
    
    // Get current date
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%Y%m%d", tm_info);
    
    const char *departments[] = {"warehouse", "manufacturing", "sales", "distribution", NULL};
    
    for (int i = 0; departments[i] != NULL; i++) {
        char expected_file[256];
        sprintf(expected_file, "%s_%s.xml", departments[i], date_str);
        
        char file_path[512];
        sprintf(file_path, "%s/%s", REPORTING_DIR, expected_file);
        
        // Check if file exists
        if (access(file_path, F_OK) != 0) {
            log_message(LOG_ERR, "Missing upload from department %s: %s", departments[i], expected_file);
        }
    }
    
    return 0;
}

// Monitor uploads directory for changes
void monitor_uploads(void) {
    static time_t last_check = 0;
    time_t now = time(NULL);
    
    // Only check every 5 minutes
    if (now - last_check < 300) {
        return;
    }
    
    last_check = now;
    
    const char *departments[] = {"warehouse", "manufacturing", "sales", "distribution", NULL};
    
    for (int i = 0; departments[i] != NULL; i++) {
        char dept_dir[256];
        sprintf(dept_dir, "%s/%s", UPLOAD_DIR, departments[i]);
        
        DIR *dir = opendir(dept_dir);
        if (!dir) {
            continue;
        }
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            char file_path[512];
            sprintf(file_path, "%s/%s", dept_dir, entry->d_name);
            
            struct stat st;
            if (stat(file_path, &st) != 0 || !S_ISREG(st.st_mode)) {
                continue;
            }
            
            // If file was modified since last check
            if (st.st_mtime > last_check - 300) {
                // Get user name from UID
                struct passwd *pw = getpwuid(st.st_uid);
                char *username = pw ? pw->pw_name : "unknown";
                
                // Log change
                FILE *log_file = fopen(CHANGE_LOG, "a");
                if (log_file) {
                    char timestamp[64];
                    struct tm *tm_info = localtime(&st.st_mtime);
                    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
                    
                    fprintf(log_file, "[%s] User '%s' modified file '%s'\n", timestamp, username, file_path);
                    fclose(log_file);
                }
            }
        }
        
        closedir(dir);
    }
}