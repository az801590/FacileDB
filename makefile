CC = gcc
CFLAGS = 
INCLUDE_DIR = ./include
SRC_DIR = ./src
EXECUTE = DB.out
OBJECTS = analyse.o block.o DB.o fileInfo.o index.o ridIndex.o

vpath %.h include
vpath %.c src

all: $(OBJECTS)
	$(CC) $^ -o $(EXECUTE)

%.o: %.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $<

# makeDir:
#   mkdir -p ./dbs

.PHONY: clean
clean:
	rm *.o

# https://stackoverflow.com/questions/30573481/path-include-and-src-directory-makefile/30602701
