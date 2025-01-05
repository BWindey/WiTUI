# Directories
SRCDIR := src
INCDIR := include
OBJDIR := obj
LIBDIR := lib
DEMODIR := demo
DEMO_OUTDIR := $(DEMODIR)/out

# Compiler and flags
CC = gcc
CFLAGS = -I$(INCDIR) -Isubmodules/WiTesting -Wall -Wextra -pedantic


# Library-file name
LIBRARY = $(LIBDIR)/libwitui.a

# Source and object files
SRC_FILES = $(wildcard $(SRCDIR)/*.c)
OBJ_FILES = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRC_FILES))

# Find all .c files in the demo/ directory
DEMO_SOURCES := $(wildcard $(DEMODIR)/*.c)
# Generate a list of executables, one for each .c file
DEMO_PROGRAMS := $(patsubst $(DEMODIR)/%.c, $(DEMO_OUTDIR)/%.out, $(DEMO_SOURCES))


# Default target to build the library
all: $(LIBRARY)

demo: $(DEMO_PROGRAMS)


# Rule to create the static library
$(LIBRARY): $(OBJ_FILES)
	@mkdir -p $(LIBDIR)
	ar rcs $@ $^

# Rule to compile object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@


# Rule to compile each .c file into its corresponding executable
$(DEMO_OUTDIR)/%.out: $(DEMODIR)/%.c $(LIBRARY)
	@mkdir -p $(DEMO_OUTDIR)
	$(CC) $(CFLAGS) -g $< -o $@ $(LIBRARY)


# Clean up
clean:
	rm -r $(OBJDIR) $(LIBDIR)
	rm -r $(DEMO_OUTDIR) 2>/dev/null


.PHONY: clean all demo
