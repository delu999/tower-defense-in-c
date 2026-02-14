# Compiler and flags
CC = clang
CFLAGS = -std=c23 -Wall -Wextra -O2
LDFLAGS = -lraylib -lm

# Debug mode: make DEBUG=1
ifdef DEBUG
CFLAGS = -std=c23 -O0 -g \
	-Wall -Wextra -Wpedantic \
	-Wconversion -Wsign-conversion \
	-Wshadow -Wdouble-promotion \
	-Wformat=2 -Wformat-overflow -Wformat-truncation \
	-Wnull-dereference -Wuninitialized \
	-Wstrict-prototypes -Wold-style-definition \
	-Wmissing-prototypes -Wmissing-declarations \
	-Wswitch-enum -Wswitch-default \
	-Wfloat-equal -Wundef \
	-Wunused -Wunused-parameter -Wunused-macros \
	-Wcast-align -Wcast-qual \
	-Wwrite-strings -Wpointer-arith \
	-Wvla -Walloca \
	-Wimplicit-fallthrough \
	-Wno-unused-function \
	-Wno-gnu-binary-literal -Wno-c23-extensions
endif

# Detect OS for platform-specific flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS
    CFLAGS += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
endif
ifeq ($(UNAME_S),Linux)
    # Linux
    LDFLAGS += -lGL -lpthread -ldl -lrt -lX11
endif

# Directories
SRC_DIR = src
BUILD_DIR = build
TARGET = tower-defense

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the game
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all clean rebuild run
