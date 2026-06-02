IDIR = include
CC = gcc
CFLAGS = -I$(IDIR)

BUILD_DIR ?= ../build/pong-tools
ODIR = $(BUILD_DIR)/obj
SDIR = src
TARGET = $(BUILD_DIR)/color_csv

LIBS = -lm

DEPS = $(IDIR)/bmp_handler.h

# Correctly tracking your reader.c and bmp_handler.c files
_OBJ = bmp_handler.o reader.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

# Fixes the Colby tutorial bug by looking inside $(SDIR)/ for the .c files
$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	@mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET) : $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

run:
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) bmp_handler *~ core
