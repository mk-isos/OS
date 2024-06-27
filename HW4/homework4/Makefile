# Makefile for N-Queens problem with multithreading

# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -pthread

# Target executable
TARGET = nqueens

# Source files
SRCS = nqueens.c stack.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Rule to link the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the build files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
