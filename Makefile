CC = gcc
CFLAGS = -Wall
LDFLAGS = -lncurses
TARGET = src/lib/ui
SOURCES = src/core/ui.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)
