#ifndef DAEMON_H
#define DAEMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

// Function declarations
void daemonize(void);
void setup_signals(void);
void signal_handler(int sig);
int check_singleton(const char *lockfile);
void write_pid_file(const char *pidfile);
void cleanup(void);

#endif /* DAEMON_H */