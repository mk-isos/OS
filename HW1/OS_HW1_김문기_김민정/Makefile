# Makefile to compile autojudge.c into an executable named autojudge

# Compiler settings - Can change these according to your system settings
CC=gcc
CFLAGS=-Wall

# Target executable name
TARGET=autojudge

# Build target
all: $(TARGET)

$(TARGET): autojudge.c
	$(CC) $(CFLAGS) autojudge.c -o $(TARGET)

