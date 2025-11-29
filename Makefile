CC = gcc
CFLAGS = -Wall -Isrc/core
LDFLAGS = -lncurses
TARGET = build/ui
SOURCES = src/core/ui.c src/core/parser.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)