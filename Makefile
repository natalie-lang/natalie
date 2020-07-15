.PHONY: test cloc debug write_build_type

CXX := c++
SRC := src
INC := include
LIB := lib/natalie
OBJ := obj
BDWGC := ext/bdwgc/include
HASHMAP := ext/hashmap/include
ONIGMO := ext/onigmo
NAT_CFLAGS ?=

# debug, coverage, or release
BUILD ?= debug

cflags.debug := -g -Wall -Wextra -Werror -Wno-unused-parameter
cflags.coverage := ${cflags.debug} -fprofile-arcs -ftest-coverage
cflags.release := -O1
CFLAGS := ${cflags.${BUILD}} ${NAT_CFLAGS}

HAS_TTY := $(shell test -t 1 && echo yes || echo no)
ifeq ($(HAS_TTY),yes)
  DOCKER_FLAGS := -i -t
endif

SOURCES := $(filter-out $(SRC)/main.cpp, $(wildcard $(SRC)/*.cpp))
OBJECTS := $(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(SOURCES))

NAT_SOURCES := $(wildcard $(SRC)/*.rb)
NAT_OBJECTS := $(patsubst $(SRC)/%.rb, $(OBJ)/nat/%.o, $(NAT_SOURCES))

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

build: write_build_type ext/bdwgc/.libs/libgccpp.a ext/hashmap/build/libhashmap.a ext/onigmo/.libs/libonigmo.a src/bridge.cpp $(OBJECTS) $(NAT_OBJECTS)

write_build_type:
	@echo $(BUILD) > .build

src/bridge.cpp: lib/natalie/compiler/bridge.rb
	ruby lib/natalie/compiler/bridge.rb > src/bridge.cpp

$(OBJ)/%.o: $(SRC)/%.cpp
	$(CXX) -std=c++17 $(CFLAGS) -I$(INC) -I$(BDWGC) -I$(HASHMAP) -I$(ONIGMO) -fPIC -c $< -o $@

$(OBJ)/nat/%.o: $(SRC)/%.rb
	bin/natalie --compile-obj $@ $<

ext/bdwgc/.libs/libgccpp.a:
	cd ext/bdwgc && ./autogen.sh && ./configure --enable-cplusplus --enable-redirect-malloc --disable-threads --enable-static --with-pic && make

ext/hashmap/build/libhashmap.a:
	mkdir -p ext/hashmap/build && cd ext/hashmap/build && CFLAGS="-fPIC" cmake .. && make

ext/onigmo/.libs/libonigmo.a:
	cd ext/onigmo && ./autogen.sh && ./configure --with-pic && make

clean:
	rm -f $(OBJ)/*.o $(OBJ)/nat/*.o

cleanall: clean clean_bdwgc clean_hashmap clean_onigmo

clean_bdwgc:
	cd ext/bdwgc && make clean || true

clean_hashmap:
	cd ext/hashmap && rm -rf build || true

clean_onigmo:
	cd ext/onigmo && make clean || true

test: build
	ruby test/all.rb

test_valgrind:
	NAT_CFLAGS="-DNAT_GC_DISABLE" make clean build
	NAT_CFLAGS="-DNAT_GC_DISABLE" bin/natalie -c assign_test test/natalie/assign_test.rb
	valgrind --leak-check=no --error-exitcode=1 ./assign_test
	NAT_CFLAGS="-DNAT_GC_DISABLE" bin/natalie -c block_spec spec/language/block_spec.rb
	valgrind --leak-check=no --error-exitcode=1 ./block_spec

test_release:
	BUILD="release" make clean test

docker_build:
	docker build -t natalie .

docker_build_clang:
	docker build -t natalie_clang --build-arg CXX=clang .

docker_test: docker_test_gcc docker_test_clang docker_test_valgrind docker_test_release

docker_test_gcc: docker_build
	docker run $(DOCKER_FLAGS) --rm --entrypoint make natalie test

docker_test_clang: docker_build_clang
	docker run $(DOCKER_FLAGS) --rm --entrypoint make natalie_clang test

docker_test_valgrind: docker_build
	docker run $(DOCKER_FLAGS) --rm --entrypoint make natalie test_valgrind

docker_test_release: docker_build
	docker run $(DOCKER_FLAGS) --rm --entrypoint make natalie test_release

cloc:
	cloc include lib src test

ctags:
	ctags -R --exclude=.cquery_cache --exclude=ext --append=no .

format:
	find include -type f -name '*.hpp' -exec clang-format -i --style=file {} +
	find src -type f -name '*.cpp' -exec clang-format -i --style=file {} +
