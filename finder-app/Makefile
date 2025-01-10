# Makefile for building the writer application
# Author: Bilal Moussa

# Compiler and flags
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Wextra -Werror -g
LDFLAGS=

# Target application and source files
TARGET=writer
SRC=$(TARGET).c
OBJ=$(SRC:.c=.o)

# Default target to build the application
.PHONY: all
all: $(TARGET)

# Build the target application
$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile the object file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove binaries and object files
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)

# Additional targets and configuration
.PHONY: help
help:
	@echo "Usage:"
	@echo "  make [all]            Build the writer application"
	@echo "  make clean            Remove all generated files"
	@echo "  make CROSS_COMPILE=<prefix> all"
	@echo "                        Cross-compile using the specified compiler prefix"
	@echo ""
	@echo "Examples:"
	@echo "  make                  Build for the native platform"
	@echo "  make CROSS_COMPILE=aarch64-none-linux-gnu-"

