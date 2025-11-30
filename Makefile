CC = gcc
CFLAGS = -Wall -Isrc/core
LDFLAGS = -lncurses
TARGET = build/ui
SOURCES = src/core/ui_main.c src/core/ui_state.c src/core/ui_tree.c src/core/ui_menu.c src/core/parser.c src/main.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)