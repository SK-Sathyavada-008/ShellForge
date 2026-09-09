CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L -Isrc

SRC = src/main.c src/parser.c src/builtins.c src/executor.c src/history.c
OBJ = $(SRC:.c=.o)
TARGET = shellforge

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
