CC = gcc
CFLAGS = -pedantic -Wall -Wextra -march=native -DDEBUG -g
LDFLAGS = -lxcb
RM = rm -rf

OUT_DIR = build
SRC_DIR = src
TARGET = featherbox

SRC = $(shell find $(SRC_DIR) -name *.c -type f)
OBJ = $(shell echo "$(SRC:.c=.o)" | sed -e "s/$(SRC_DIR)\//$(OUT_DIR)\//g")

$(OUT_DIR)/$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(OBJ): $(OUT_DIR)/%.o: $(SRC_DIR)/%.c | $(OUT_DIR)/
	$(CC) $(CFLAGS) -c $< -o $@ 

$(OUT_DIR)/:
	mkdir -p $@

.PHONY: clean
clean:
	$(RM) $(OUT_DIR)
