# Compiler and flags
CC = clang
CFLAGS = -I./include -I./include/util -Wall -g

# Source and target directories
SRCDIR = src
OBJDIR = bin
INCLUDEDIR = include

# Main executable target name
TARGET = $(OBJDIR)/Facile

# Test executable target names
TEST_INDEX_TARGET = $(OBJDIR)/FacileDB_Index_Test
TEST_DB_TARGET = $(OBJDIR)/FacileDB_Db_Test

# Source files for main target
SRC = $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/util/*.c)

# Object files for main target
OBJ = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))

# Default target to build main executable
all: $(OBJDIR) $(TARGET)

# Main executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Test target to compile test_main.c and test_db_main.c
test: $(OBJDIR) $(TEST_INDEX_TARGET) $(TEST_DB_TARGET)

# Test executable for test_index_main.c
$(TEST_INDEX_TARGET): $(SRCDIR)/test/test_index_main.c
	$(CC) $(CFLAGS) -I./src -I./src/util $(SRCDIR)/test/test_index_main.c -o $(TEST_INDEX_TARGET)

# Test executable for test_db_main.c
$(TEST_DB_TARGET): $(SRCDIR)/test/test_faciledb_main.c
	$(CC) $(CFLAGS) -I./src -I./src/util $(SRCDIR)/test/test_faciledb_main.c -o $(TEST_DB_TARGET)

# Ensure bin directory exists
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Compile .c files to .o files, creating directories if needed
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up all build artifacts
clean:
	rm -rf $(OBJDIR)/*.o $(OBJDIR)/*/*.o $(TARGET) $(TEST_INDEX_TARGET) $(TEST_DB_TARGET)

# Phony targets
.PHONY: all test clean