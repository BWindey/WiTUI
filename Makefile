# Directories
SRCDIR := src
INCDIR := include
OBJDIR := obj
LIBDIR := lib
DEMODIR := demo

# Compiler and flags
CC = gcc
CFLAGS = -I$(INCDIR) -Isubmodules/WiTesting -Wall -Wextra -pedantic


# Library-file name
LIBRARY = $(LIBDIR)/libwitui.a

# Source and object files
SRC_FILES = $(wildcard $(SRCDIR)/*.c)
OBJ_FILES = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRC_FILES))


# Default target to build the library
all: $(LIBRARY)

demo: $(LIBRARY)
	$(CC) $(CFLAGS) -g $(wildcard $(DEMODIR)/*.c) -o demo.out $(LIBRARY)

# Rule to create the static library
$(LIBRARY): $(OBJ_FILES) | $(LIBDIR)
	ar rcs $@ $^

# Rule to compile object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@


# Ensure lib directory exists
$(LIBDIR):
	mkdir -p $(LIBDIR)

# Ensure obj directory exists
$(OBJDIR):
	mkdir -p $(OBJDIR)


# Clean up
clean:
	rm -r $(OBJDIR) $(LIBDIR) demo.out


.PHONY: clean all demo
