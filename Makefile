CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -std=c99

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests

TARGET = $(BIN_DIR)/server
TEST_TARGET = $(BIN_DIR)/test_server

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/server.c $(SRC_DIR)/log.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: dirs $(TARGET) $(TEST_TARGET)

dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): $(TEST_DIR)/test.c
	$(CC) $(CFLAGS) $< -o $@ -lpthread

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean dirs