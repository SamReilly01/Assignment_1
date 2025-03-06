# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lrt

# Directories
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin
INIT_DIR = init.d

# Source files
DAEMON_SRC = $(SRC_DIR)/daemon.c $(SRC_DIR)/file_ops.c $(SRC_DIR)/ipc.c
CONTROL_SRC = $(SRC_DIR)/control.c

# Target executables
DAEMON = company_daemon
CONTROL = company_control

# Default target
all: prepare $(BIN_DIR)/$(DAEMON) $(BIN_DIR)/$(CONTROL)

# Create bin directory
prepare:
	@mkdir -p $(BIN_DIR)

# Compile daemon
$(BIN_DIR)/$(DAEMON): $(DAEMON_SRC)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $^ $(LDFLAGS)

# Compile control program
$(BIN_DIR)/$(CONTROL): $(CONTROL_SRC)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $^ $(LDFLAGS)

# Install
install: all
	@echo "Installing daemon..."
	@install -m 755 $(BIN_DIR)/$(DAEMON) /usr/sbin/
	@install -m 755 $(BIN_DIR)/$(CONTROL) /usr/sbin/
	@install -m 755 $(INIT_DIR)/$(DAEMON) /etc/init.d/
	@mkdir -p /var/company/upload/warehouse
	@mkdir -p /var/company/upload/manufacturing
	@mkdir -p /var/company/upload/sales
	@mkdir -p /var/company/upload/distribution
	@mkdir -p /var/company/reporting
	@mkdir -p /var/company/backup
	@mkdir -p /var/company/logs
	@echo "Enabling daemon at boot time..."
	@if [ -x /usr/sbin/update-rc.d ]; then \
		update-rc.d $(DAEMON) defaults; \
	elif [ -x /usr/sbin/chkconfig ]; then \
		chkconfig --add $(DAEMON); \
	else \
		echo "WARNING: Could not enable daemon at boot time. Unknown system type."; \
	fi
	@echo "Installation complete."

# Uninstall
uninstall:
	@echo "Uninstalling daemon..."
	@if [ -x /usr/sbin/update-rc.d ]; then \
		update-rc.d -f $(DAEMON) remove; \
	elif [ -x /usr/sbin/chkconfig ]; then \
		chkconfig --del $(DAEMON); \
	fi
	@rm -f /usr/sbin/$(DAEMON)
	@rm -f /usr/sbin/$(CONTROL)
	@rm -f /etc/init.d/$(DAEMON)
	@echo "Uninstallation complete."

# Clean
clean:
	@rm -rf $(BIN_DIR)

.PHONY: all prepare install uninstall clean