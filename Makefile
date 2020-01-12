.PHONY: test cloc

SRC := src
LIB := lib/natalie
OBJ := obj

SOURCES := $(filter-out $(SRC)/main.c, $(wildcard $(SRC)/*.c))
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

build: $(OBJECTS) $(OBJ)/language/exceptions.o

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -g -I$(SRC) -c $< -o $@

$(OBJ)/language/exceptions.o: $(LIB)/language/exceptions.nat
	bin/natalie --compile-obj $@ $<

clean:
	rm -f $(OBJ)/*.o $(OBJ)/language/*.o

test: build
	ruby test/all.rb

cloc:
	cloc --not-match-f=hashmap.* --exclude-dir=.cquery_cache .
