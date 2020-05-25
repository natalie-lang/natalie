.PHONY: test cloc debug write_build_type

SRC := src
INC := include
LIB := lib/natalie
OBJ := obj
HASHMAP := ext/hashmap/include
ONIGMO := ext/onigmo
NAT_CFLAGS ?=

# debug, coverage, or release
BUILD ?= debug

cflags.debug := -g -Wall -Wextra -Werror -Wno-unused-parameter -D"NAT_GC_COLLECT_DEBUG=true"
cflags.coverage := ${cflags.debug} -fprofile-arcs -ftest-coverage
cflags.release := -O1
CFLAGS := ${cflags.${BUILD}} ${NAT_CFLAGS}

HAS_TTY := $(shell test -t 1 && echo yes || echo no)
ifeq ($(HAS_TTY),yes)
  DOCKER_FLAGS := -i -t
endif

SOURCES := $(filter-out $(SRC)/main.cpp, $(wildcard $(SRC)/*.cpp))
OBJECTS := $(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(SOURCES))

NAT_SOURCES := $(wildcard $(SRC)/*.nat)
NAT_OBJECTS := $(patsubst $(SRC)/%.nat, $(OBJ)/nat/%.o, $(NAT_SOURCES))

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

build: write_build_type ext/hashmap/build/libhashmap.a ext/onigmo/.libs/libonigmo.a $(OBJECTS) $(NAT_OBJECTS)

write_build_type:
	@echo $(BUILD) > .build

$(OBJ)/%.o: $(SRC)/%.cpp
	$(CC) -std=c++17 $(CFLAGS) -I$(INC) -I$(HASHMAP) -I$(ONIGMO) -fPIC -c $< -o $@

$(OBJ)/nat/%.o: $(SRC)/%.nat
	bin/natalie --compile-obj $@ $<

ext/hashmap/build/libhashmap.a:
	mkdir ext/hashmap/build && cd ext/hashmap/build && CFLAGS="-fPIC" cmake .. && make

ext/onigmo/.libs/libonigmo.a:
	cd ext/onigmo && ./autogen.sh && ./configure --with-pic && make

clean:
	rm -f $(OBJ)/*.o $(OBJ)/nat/*.o

cleanall: clean clean_hashmap clean_onigmo

clean_hashmap:
	cd ext/hashmap && rm -rf build || true

clean_onigmo:
	cd ext/onigmo && make clean || true

test: build
	ruby test/all.rb

test_valgrind: build
	bin/natalie -c assign_test test/natalie/assign_test.nat
	valgrind --leak-check=no --suppressions=test/valgrind-suppressions --error-exitcode=1 ./assign_test
	bin/natalie -c block_spec spec/language/block_spec.nat
	valgrind --leak-check=no --suppressions=test/valgrind-suppressions --error-exitcode=1 ./block_spec

test_release:
	BUILD="release" make clean test

test_release_slow:
	BUILD="release" NAT_CFLAGS="-D\"NAT_GC_COLLECT_DEBUG=true\"" make clean test

coverage_report:
	lcov -c --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage-report

docker_build:
	docker build -t natalie .

docker_build_clang:
	docker build -t natalie_clang --build-arg CC=clang .

docker_test: docker_test_gcc docker_test_clang docker_test_valgrind docker_test_release docker_test_release_slow

docker_test_gcc: docker_build
	docker run $(DOCKER_FLAGS) --rm --entrypoint make natalie test

docker_test_clang: docker_build_clang
	docker run $(DOCKER_FLAGS) --rm --entrypoint make natalie_clang test

docker_test_valgrind: docker_build
	docker run $(DOCKER_FLAGS) --rm --entrypoint make natalie test_valgrind

docker_test_release: docker_build
	docker run $(DOCKER_FLAGS) --rm --entrypoint make natalie test_release

docker_test_release_slow: docker_build
	docker run $(DOCKER_FLAGS) --rm --entrypoint make natalie test_release_slow

docker_coverage_report: docker_build
	rm -rf coverage-report
	mkdir coverage-report
	docker run $(DOCKER_FLAGS) -v $(CURDIR)/coverage-report:/natalie/coverage-report --rm --entrypoint bash natalie -c "make BUILD=coverage clean test; make coverage_report"

cloc:
	cloc --not-match-f=hashmap.\* --not-match-f=compile_commands.json --exclude-dir=.cquery_cache,.github,ext .

ctags:
	ctags -R --exclude=.cquery_cache --exclude=ext --append=no .

format:
	find include -type f -name '*.h' -exec clang-format -i --style=file {} +
	find src -type f -name '*.cpp' -exec clang-format -i --style=file {} +
