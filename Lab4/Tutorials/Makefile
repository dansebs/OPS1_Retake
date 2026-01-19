# Compiler setup
CC = gcc
CFLAGS = -Wall -pthread

# Output binary name
TARGET = dicegame

# Source file
SRC = main.c

# Default rule: Build the target
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Run rule: Compile and execute immediately
run: $(TARGET)
	./$(TARGET)

# Clean rule: Remove the executable
clean:
	rm -f $(TARGET)