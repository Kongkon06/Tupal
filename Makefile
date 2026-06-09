# Makefile for clox interpreter (Embarcadero compatible)

CFLAGS = -Wall -Wextra -std=c99 -g -Wno-unused-parameter -I$(SRCDIR)
CFLAGS_RELEASE = -Wall -Wextra -std=c99 -O2 -I$(SRCDIR)

TARGET = tupal
OBJDIR = build
SRCDIR = src

SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/vm.c \
          $(SRCDIR)/compiler.c \
          $(SRCDIR)/scanner.c \
          $(SRCDIR)/memory.c \
          $(SRCDIR)/object.c \
          $(SRCDIR)/table.c \
          $(SRCDIR)/value.c \
          $(SRCDIR)/chunk.c \
          $(SRCDIR)/debug.c


HEADERS = \
    $(SRCDIR)/common.h \
    $(SRCDIR)/chunk.h \
    $(SRCDIR)/debug.h \
    $(SRCDIR)/vm.h \
    $(SRCDIR)/compiler.h \
    $(SRCDIR)/scanner.h \
    $(SRCDIR)/memory.h \
    $(SRCDIR)/object.h \
    $(SRCDIR)/table.h \
    $(SRCDIR)/value.h

OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))


# Default target
all: $(OBJDIR) $(TARGET)

# Create build folder (POSIX)
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Link
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Release
release: CFLAGS = $(CFLAGS_RELEASE)
release: clean $(TARGET)

# Debug
debug: CFLAGS = $(CFLAGS) -DDEBUG
debug: clean $(TARGET)

# Run
run: $(TARGET)
	./$(TARGET)

# Clean
clean:
	rm -f $(OBJDIR)/*.o

distclean: clean
	rm -f $(TARGET)