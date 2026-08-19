CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -Isrc

SRC = src/main.c src/parser.c src/builtins.c src/executor.c
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
