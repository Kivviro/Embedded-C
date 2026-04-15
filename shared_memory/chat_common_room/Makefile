CC = gcc
CFLAGS = -std=gnu11 -O2 -Iinclude
LDFLAGS_CLIENT = -lrt -pthread -lncurses
LDFLAGS_SERVER = -lrt -pthread

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

CLIENT_SRCS = $(SRC_DIR)/client_core.c $(SRC_DIR)/client_ui.c
SERVER_SRCS = $(SRC_DIR)/server.c

CLIENT_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(CLIENT_SRCS))
SERVER_OBJ  = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SERVER_SRCS))

CLIENT_BIN = $(BIN_DIR)/client.out
SERVER_BIN = $(BIN_DIR)/server.out

OBJS = client_core.o client_ui.o

.PHONY: all clean dirs

all: dirs $(SERVER_BIN) $(CLIENT_BIN)

dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(CLIENT_BIN): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS_CLIENT)

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS_SERVER)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

