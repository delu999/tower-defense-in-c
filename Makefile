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
EDITOR_DIR = tools/editor
TARGET = tower-defense
EDITOR_TARGET = level-editor

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

EDITOR_SRCS = $(EDITOR_DIR)/main.c $(EDITOR_DIR)/editor.c
EDITOR_OBJS = $(BUILD_DIR)/editor_main.o $(BUILD_DIR)/editor.o

# Default target
all: $(BUILD_DIR) $(TARGET)

# Editor target
editor: $(BUILD_DIR) $(EDITOR_TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Build editor executable
$(EDITOR_TARGET): $(EDITOR_OBJS)
	$(CC) $(EDITOR_OBJS) -o $(EDITOR_TARGET) $(LDFLAGS)
	@echo "Editor build complete: $(EDITOR_TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile editor files
$(BUILD_DIR)/editor_main.o: $(EDITOR_DIR)/main.c
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/editor.o: $(EDITOR_DIR)/editor.c
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

# Run the game
run: $(TARGET)
	./$(TARGET)

# Run the editor
run-editor: $(EDITOR_TARGET)
	./$(EDITOR_TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(EDITOR_TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all clean rebuild run editor run-editor
