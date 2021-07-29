CC = gcc
OUT_DIR = build
SRC_DIR = src
TARGET = featherbox
CFLAGS = -pedantic -Wall -Wextra -march=native -DDEBUG -g
LDFLAGS = -lxcb
INCLUDES = -I$(SRC_DIR)
RM = rm -rf

SRC = $(shell find $(SRC_DIR) -name *.c -type f)
OBJ = $(shell echo "$(SRC:.c=.o)" | sed -e "s/$(SRC_DIR)\//$(OUT_DIR)\//g")

$(OUT_DIR)/$(TARGET): $(OBJ)
	$(CC) $< -o $@ $(LDFLAGS)

$(OBJ): $(OUT_DIR)/%.o: $(SRC_DIR)/%.c | $(OUT_DIR)/
	$(CC) $(CFLAGS) -c $< -o $@ 

$(OUT_DIR)/:
	mkdir -p $@

clean:
	$(RM) $(OUT_DIR)

format:
	find . -regex '.*\.\(c\|h\)' -exec clang-format -style=file -i {} \; 

.PHONY: clean format
