# Compiler and flags
CC = clang
CFLAGS = -I./include -I./include/util -Wall -g

# Source and target directories
SRCDIR = src
OBJDIR = bin
INCLUDEDIR = include

# Main executable target name
TARGET = $(OBJDIR)/Facile

# Test executable target name
TEST_TARGET = $(OBJDIR)/Facile_Test

# Source files for main target
SRC = $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/util/*.c)

# Object files for main target
OBJ = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))

# Default target to build main executable
all: $(TARGET)

# Main executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Test target to only compile test_main.c with additional include path
test: $(TEST_TARGET)

# Test executable only compiling test_main.c with ./src/util include path
$(TEST_TARGET): $(SRCDIR)/test_main.c
	$(CC) $(CFLAGS) -I./src/util $(SRCDIR)/test_main.c -o $(TEST_TARGET)

# Compile .c files to .o files, creating directories if needed
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up all build artifacts
clean:
	rm -rf $(OBJDIR)/*.o $(OBJDIR)/*/*.o $(TARGET) $(TEST_TARGET)

# Phony targets
.PHONY: all test clean