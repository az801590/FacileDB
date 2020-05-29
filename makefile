CC = gcc
CFLAGS = -Wall -Werror -Wextra -std=c99 -pedantic
INCLUDE_DIR = ./include
SRC_DIR = ./src
EXECUTE = Facile
OBJECTS = DB.o index.o main.o

vpath %.h include
vpath %.c src

all: $(OBJECTS)
	$(CC) $^ -o $(EXECUTE)

%.o: %.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $<

.PHONY: clean
clean:
	rm *.o
