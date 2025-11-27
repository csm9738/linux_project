CC = gcc
CFLAGS = -Wall -Isrc/core
SOURCES = src/core/ui.c src/core/parser.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)
