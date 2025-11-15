CC=gcc
CFLAGS=-g -Wall -Wextra -fsanitize=signed-integer-overflow
CLIBS=-lX11 -lcrypt

PROGRAMS = txstart txbar txlock
BUILD_DIR = build

TARGETS = $(addprefix $(BUILD_DIR)/, $(PROGRAMS))

all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/txstart: utils/txstart.c core.c
	$(CC) $(CFLAGS) -I. $^ -o $@ $(CLIBS)

$(BUILD_DIR)/txbar: utils/txbar.c core.c
	$(CC) $(CFLAGS) -I. $^ -o $@ $(CLIBS)

$(BUILD_DIR)/txlock: utils/txlock.c core.c
	$(CC) $(CFLAGS) -I. $^ -o $@ $(CLIBS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
