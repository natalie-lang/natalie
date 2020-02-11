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

NAT_SOURCES := $(wildcard $(SRC)/*.nat)
NAT_OBJECTS := $(patsubst $(SRC)/%.nat, $(OBJ)/nat/%.o, $(NAT_SOURCES))

build: write_build_type $(OBJECTS) $(NAT_OBJECTS)

write_build_type:
	@echo $(BUILD) > .build

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

$(OBJ)/nat/%.o: $(SRC)/%.nat
	bin/natalie --compile-obj $@ $<

clean:
	rm -f $(OBJ)/*.o $(OBJ)/nat/*.o

test: build
	ruby test/all.rb

cloc:
	cloc --not-match-f=hashmap.* --exclude-dir=.cquery_cache .
