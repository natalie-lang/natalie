.PHONY: test cloc debug write_build_type clean clean_nat

SRC := src
LIB := lib/natalie
OBJ := obj
ONIGMO := ext/onigmo

BUILD := debug

cflags.debug := -g -Wall -Wextra -Werror -Wno-unused-parameter -pthread
cflags.coverage := ${cflags.debug} -fprofile-arcs -ftest-coverage -pthread
cflags.release := -O3 -pthread
CFLAGS := ${cflags.${BUILD}}

SOURCES := $(filter-out $(SRC)/main.c, $(wildcard $(SRC)/*.c))
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

NAT_SOURCES := $(wildcard $(SRC)/*.nat)
NAT_OBJECTS := $(patsubst $(SRC)/%.nat, $(OBJ)/nat/%.o, $(NAT_SOURCES))

build: write_build_type ext/onigmo/.libs/libonigmo.a $(OBJECTS) $(NAT_OBJECTS)

write_build_type:
	@echo $(BUILD) > .build

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -I$(ONIGMO) -fPIC -c $< -o $@

$(OBJ)/nat/%.o: $(SRC)/%.nat
	bin/natalie --compile-obj $@ $<

ext/onigmo/.libs/libonigmo.a:
	cd ext/onigmo && ./autogen.sh && ./configure --with-pic && make

clean_nat:
	rm -f $(OBJ)/*.o $(OBJ)/nat/*.o

clean: clean_nat
	cd ext/onigmo && make clean || true

test: build
	ruby test/all.rb

test_in_ubuntu:
	docker build -t natalie .
	docker run -i -t --rm --entrypoint make natalie test

cloc:
	cloc --not-match-f=hashmap.\* --not-match-f=compile_commands.json --exclude-dir=.cquery_cache,.github,ext .
