CC=gcc
CSA=scan-build

CFLAGS = -c -std=gnu99 -Wall -Wextra -ggdb3

SOURCES = $(shell find src -name "*.c")
HEADER_FILES = $(shell find src -name "*.h")
OBJECTS = $(SOURCES:.c=.o)
BUILD_DIR = build

TARGET=stil

all: $(TARGET)


$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)
	@mkdir -p $(BUILD_DIR)
	@mv $(OBJECTS) $(BUILD_DIR)

%.o: %.c $(HEADER_FILES)
	$(CC) $(CFLAGS) -o2 -o $@ $<
csa:
	$(CSA) $(CC) $(CFLAGS) $(SOURCES)

clean:
	rm -rf src/*.o $(TARGET)
