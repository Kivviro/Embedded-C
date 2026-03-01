CC      := gcc
CFLAGS  := -std=c11 -Iinclude
LDFLAGS := -lncursesw -lmagic

CFLAGS += -MMD -MP
-include $(OBJS:.o=.d)

SRC_DIR := src
OBJ_DIR := build
TARGET  := kfm.out


SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

re: clean all

.PHONY: all clean re
