# Variables
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O2 -g -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lpthread

# Final executable name
TARGET = log_sanitizer

# Source and object files
SRCS = main.c
OBJS = $(SRCS:.c=.o)

# Default rule
all: $(TARGET)

# Linking
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compiling
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Utility to clean up the directory
clean:
	rm -f $(TARGET) $(OBJS) errors.log clean.log

# Phony targets
.PHONY: all clean