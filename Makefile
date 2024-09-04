# Compiler and flags
CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -pedantic

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
LIBDIR = lib

# Files
SRC = $(SRCDIR)/wiTUI.c
OBJ = $(OBJDIR)/wiTUI.o
LIB = $(LIBDIR)/libwitui.a

# Default target to build the library
all: $(LIB)

# Rule to create the static library
$(LIB): $(OBJ) | $(LIBDIR)
	ar rcs $@ $^

# Rule to compile object file
$(OBJ): $(SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure lib directory exists
$(LIBDIR):
	mkdir -p $(LIBDIR)

# Ensure obj directory exists
$(OBJDIR):
	mkdir -p $(OBJDIR)


# Clean up
clean:
	rm -rf $(OBJDIR) $(LIBDIR)


.PHONY: clean all
