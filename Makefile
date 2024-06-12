TARGET_EXEC := potato
BUILD_DIR := ./build
SRC_DIR := ./src
INCLUDE_DIR := ./headers

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CC := gcc
CFLAGS := -g -I $(INCLUDE_DIR)

$(TARGET_EXEC): $(OBJS)
	$(CC) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)/*

