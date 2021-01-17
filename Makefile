.PHONY: build build_debug build_release test cloc debug write_build_type

DOCKER_FLAGS ?= -i -t
GNUMAKEFLAGS := --no-print-directory

build: build_debug

build_debug:
	cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -S . -B build -DCMAKE_BUILD_TYPE="Debug" -DCMAKE_MAKE_PROGRAM="${MAKE}"
	cmake --build build -j 4
	cp build/compile_commands.json .

build_release:
	cmake -S . -B build -DCMAKE_BUILD_TYPE="Release"
	cmake --build build -j 4

clean:
	if [ -d build ]; then make -C build -f CMakeFiles/Makefile2 CMakeFiles/natalie-base.dir/clean; fi

cleanall:
	rm -rf build

test: build
	bundle exec ruby test/all.rb

test_valgrind:
	NAT_CXX_FLAGS="-DNAT_GC_DISABLE" make clean build
	NAT_CXX_FLAGS="-DNAT_GC_DISABLE" bin/natalie -c assign_test test/natalie/assign_test.rb
	valgrind --leak-check=no --error-exitcode=1 ./assign_test
	NAT_CXX_FLAGS="-DNAT_GC_DISABLE" bin/natalie -c block_spec spec/language/block_spec.rb
	valgrind --leak-check=no --error-exitcode=1 ./block_spec

docker_build:
	docker build -t natalie .

docker_bash: docker_build
	docker run -it --rm --entrypoint bash natalie

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
	docker run $(DOCKER_FLAGS) --rm --entrypoint make natalie clean build_release test

cloc:
	cloc include lib src test

ctags:
	ctags -R --exclude=.cquery_cache --exclude=ext --append=no .

format:
	find include -type f -name '*.hpp' -exec clang-format -i --style=file {} +
	find src -type f -name '*.cpp' -exec clang-format -i --style=file {} +
