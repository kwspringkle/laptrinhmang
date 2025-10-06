CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = ltm_week1
SOURCES = main.c utils.c
HEADERS = utils.h
OBJECTS = $(SOURCES:.c=.o)
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJECTS) $(TARGET) $(TARGET).exe
clean-win:
	del $(OBJECTS) $(TARGET).exe
run: $(TARGET)
	./$(TARGET)
debug: CFLAGS += -DDEBUG
debug: $(TARGET)
release: CFLAGS += -O2 -DNDEBUG
release: clean $(TARGET)
help:
	@echo "Available targets:"
	@echo "  all      - Build the program (default)"
	@echo "  clean    - Remove all generated files (Linux/Mac)"
	@echo "  clean-win- Remove all generated files (Windows)"
	@echo "  run      - Build and run the program"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build optimized release version"
	@echo "  help     - Show this help message"
.PHONY: all clean clean-win run debug release help