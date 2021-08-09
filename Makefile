CC = gcc
CFLAGS = -pedantic -Wall -Wextra -march=native -DDEBUG -g
LDFLAGS = -lxcb -lxcb-keysyms -lconfig
RM = rm -rf

SRC = fleon.c
OBJ = $(SRC:.c=.o)
TARGET = fleon

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	$(RM) $(OBJ) $(TARGET)

format:
	find . -regex '.*\.\(c\|h\)' -exec clang-format -style=file -i {} \; 

.PHONY: clean format
