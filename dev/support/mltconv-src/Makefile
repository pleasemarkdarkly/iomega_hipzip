OBJ := main.o memmap.o

ifeq ($(OSTYPE),cygwin)
	TARGET_EXE := mltconv.exe
else
	TARGET_EXE := mltconv
endif

DERIVED := $(OBJ) $(TARGET_EXE)

all: $(TARGET_EXE)

$(TARGET_EXE): $(OBJ)
	g++ -o $(TARGET_EXE) $(OBJ)

clean:
	rm -f $(DERIVED)
