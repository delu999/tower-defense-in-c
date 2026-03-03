CC = clang
CFLAGS = -std=c23 -Wall -Wextra -O2
LDFLAGS = -lraylib -lm

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

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS += -I/opt/homebrew/include
	LDFLAGS += -L/opt/homebrew/lib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
endif
ifeq ($(UNAME_S),Linux)
	LDFLAGS += -lGL -lpthread -ldl -lrt -lX11
endif

BUILD_DIR = ../build/game
TARGET = ../tower-defense

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	cd .. && ./tower-defense

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

rebuild: clean all

-include $(DEPS)

.PHONY: all run clean rebuild
