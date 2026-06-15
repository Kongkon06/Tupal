# Makefile for clox interpreter (Embarcadero compatible)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -Wno-unused-parameter
CFLAGS_RELEASE = -Wall -Wextra -std=c99 -O2

TARGET = tupal.exe
OBJDIR = build

OBJECTS = $(OBJDIR)\main.o $(OBJDIR)\vm.o $(OBJDIR)\compiler.o \
          $(OBJDIR)\scanner.o $(OBJDIR)\memory.o $(OBJDIR)\object.o \
          $(OBJDIR)\table.o $(OBJDIR)\value.o $(OBJDIR)\chunk.o \
          $(OBJDIR)\debug.o $(OBJDIR)\graphics.o

HEADERS = common.h chunk.h debug.h vm.h compiler.h scanner.h memory.h object.h table.h value.h graphics.h

# Default target
all: $(OBJDIR) $(TARGET)

# Create build folder (Windows style)
$(OBJDIR):
	if not exist $(OBJDIR) mkdir $(OBJDIR)

# Link
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) -lgdi32

# Compile rules
$(OBJDIR)\main.o: main.c $(HEADERS)
	$(CC) $(CFLAGS) -c main.c -o $(OBJDIR)\main.o

$(OBJDIR)\vm.o: vm.c $(HEADERS)
	$(CC) $(CFLAGS) -c vm.c -o $(OBJDIR)\vm.o

$(OBJDIR)\compiler.o: compiler.c $(HEADERS)
	$(CC) $(CFLAGS) -c compiler.c -o $(OBJDIR)\compiler.o

$(OBJDIR)\scanner.o: scanner.c $(HEADERS)
	$(CC) $(CFLAGS) -c scanner.c -o $(OBJDIR)\scanner.o

$(OBJDIR)\memory.o: memory.c $(HEADERS)
	$(CC) $(CFLAGS) -c memory.c -o $(OBJDIR)\memory.o

$(OBJDIR)\object.o: object.c $(HEADERS)
	$(CC) $(CFLAGS) -c object.c -o $(OBJDIR)\object.o

$(OBJDIR)\table.o: table.c $(HEADERS)
	$(CC) $(CFLAGS) -c table.c -o $(OBJDIR)\table.o

$(OBJDIR)\value.o: value.c $(HEADERS)
	$(CC) $(CFLAGS) -c value.c -o $(OBJDIR)\value.o

$(OBJDIR)\chunk.o: chunk.c $(HEADERS)
	$(CC) $(CFLAGS) -c chunk.c -o $(OBJDIR)\chunk.o

$(OBJDIR)\debug.o: debug.c $(HEADERS)
	$(CC) $(CFLAGS) -c debug.c -o $(OBJDIR)\debug.o

$(OBJDIR)\graphics.o: graphics.c $(HEADERS)
	$(CC) $(CFLAGS) -c graphics.c -o $(OBJDIR)\graphics.o

# Release
release: CFLAGS = $(CFLAGS_RELEASE)
release: clean $(TARGET)

# Debug
debug: CFLAGS = $(CFLAGS) -DDEBUG
debug: clean $(TARGET)

# Run
run: $(TARGET)
	$(TARGET)

# Clean
clean:
	if exist $(OBJDIR)\*.o del /Q $(OBJDIR)\*.o

distclean: clean
	if exist $(TARGET) del /Q $(TARGET)