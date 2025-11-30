CC = gcc
CFLAGS = -Wall -Isrc/core
LDFLAGS = -lncurses
TARGET = build/ui
SOURCES = src/core/start_ui.c src/core/ui_state.c src/core/ui_left.c src/core/ui_right.c src/core/parser.c src/main.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)