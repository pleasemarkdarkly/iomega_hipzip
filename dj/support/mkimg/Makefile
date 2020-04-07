.PHONY: all

OBJECTS := mkimg.o imgutil.o
CFLAGS := -g -O2

all: $(OBJECTS)
	gcc $(OBJECTS) -o mkimg.exe


clean:
	rm -f $(OBJECTS)