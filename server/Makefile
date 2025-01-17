# Makefile for aesdsocket

CC := gcc
CFLAGS := -Wall -Wextra -g
TARGET := aesdsocket
SRCS := aesdsocket.c
OBJS := $(SRCS:.c=.o)
CROSS_COMPILE ?=

# Output directory for the compiled binary
BINDIR := ./server
BIN := $(TARGET)

.PHONY: all clean help

all: $(BIN)

# Compile the target binary
$(BIN): $(OBJS)
	#@mkdir -p $(BINDIR)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $@ $^

# Compile object files
%.o: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(BIN)

help:
	@echo "Makefile for aesdsocket"
	@echo "Usage:"
	@echo "  make all          - Build the aesdsocket binary"
	@echo "  make clean        - Remove all generated files and directories"
	@echo "  make help         - Display this help message"
	@echo "Variables:"
	@echo "  CROSS_COMPILE     - Prefix for cross-compilation tools (e.g., aarch64-none-linux-gnu-)"

# Default target to allow cross-compilation
.DEFAULT_GOAL := all
