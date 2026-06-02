IDIR = include
CC = gcc
CFLAGS = -I$(IDIR)

ODIR = obj
SDIR = src
TARGET = color_csv

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
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

run:
	./$(TARGET)

clean:
	rm -rf $(ODIR) bmp_handler *~ core
