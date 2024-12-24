# Compiler and flags
CC = gcc
CFLAGS = -I$(INCDIR) -I$(SUBDIR)/WiTesting -Wall -Wextra -pedantic

# Directories
SRCDIR := src
INCDIR := include
OBJDIR := obj
LIBDIR := lib
SUBDIR := submodules

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

test:
	$(CC) $(CFLAGS) -g $(SRCDIR)/*.c -o test.out

# Clean up
clean:
	rm -rf $(OBJDIR) $(LIBDIR) test.out


.PHONY: clean all test
