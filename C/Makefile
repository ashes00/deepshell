# DeepShell C Version Makefile
# Supports both Linux and Windows (MSYS2/Mingw64)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = 
LIBS = -lcurl -ljson-c -lpthread

# Platform detection
ifeq ($(OS),Windows_NT)
    # Windows with MSYS2/Mingw64
    EXECUTABLE = deepshell.exe
    CFLAGS += -D_WIN32
    LDFLAGS += -static-libgcc
else
    # Linux
    EXECUTABLE = deepshell
    CFLAGS += -D_POSIX_C_SOURCE=200809L
endif

# Source files
SOURCES = main.c settings.c gemini.c ollama.c openrouter.c utils.c config.c interactive.c
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(EXECUTABLE)

# Build the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS) $(LIBS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

# Install dependencies (Linux)
install-deps-linux:
	sudo apt-get update
	sudo apt-get install -y libcurl4-openssl-dev libjson-c-dev

# Install dependencies (Windows/MSYS2)
install-deps-windows:
	pacman -S mingw-w64-x86_64-curl mingw-w64-x86_64-json-c

# Development build with debug info
debug: CFLAGS += -g -DDEBUG
debug: $(EXECUTABLE)

# Release build
release: CFLAGS += -DNDEBUG
release: $(EXECUTABLE)

.PHONY: all clean install-deps-linux install-deps-windows debug release 