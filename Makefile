CC = gcc
CFLAGS = -Wall -Isrc/core -g
LDFLAGS = -lncurses
SOURCES = src/core/ui.c
TARGET = src/lib/ui

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
