.PHONY: test cloc debug write_build_type

SRC := src
LIB := lib/natalie
OBJ := obj

BUILD := debug

cflags.debug := -g
cflags.release := -O3
CFLAGS := ${cflags.${BUILD}}

SOURCES := $(filter-out $(SRC)/main.c, $(wildcard $(SRC)/*.c))
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

build: write_build_type $(OBJECTS) $(OBJ)/language/exceptions.o

write_build_type:
	@echo $(BUILD) > .build

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

$(OBJ)/language/exceptions.o: $(LIB)/language/exceptions.nat
	bin/natalie --compile-obj $@ $<

clean:
	rm -f $(OBJ)/*.o $(OBJ)/language/*.o

test: build
	ruby test/all.rb

cloc:
	cloc --not-match-f=hashmap.* --exclude-dir=.cquery_cache .
