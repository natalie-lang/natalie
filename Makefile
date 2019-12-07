.PHONY: test cloc

SRC := src
OBJ := obj

SOURCES := $(filter-out $(SRC)/main.c, $(wildcard $(SRC)/*.c))
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

build: $(OBJECTS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -I$(SRC) -c $< -o $@

clean:
	rm -f $(OBJ)/*.o

test: build
	ruby test/all.rb

cloc:
	cloc --not-match-f=hashmap.* .
