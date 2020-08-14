.PHONY: build test cloc debug write_build_type

DOCKER_FLAGS ?= -i -t

build:
	cmake -S . -B build
	cmake --build build -j 4

clean:
	cd build && make -f CMakeFiles/Makefile2 CMakeFiles/natalie.dir/clean

cleanall:
	cmake --build build --target clean

test: build
	ruby test/all.rb

test_valgrind:
	NAT_CXX_FLAGS="-DNAT_GC_DISABLE" make clean build
	NAT_CXX_FLAGS="-DNAT_GC_DISABLE" bin/natalie -c assign_test test/natalie/assign_test.rb
	valgrind --leak-check=no --error-exitcode=1 ./assign_test
	NAT_CXX_FLAGS="-DNAT_GC_DISABLE" bin/natalie -c block_spec spec/language/block_spec.rb
	valgrind --leak-check=no --error-exitcode=1 ./block_spec

test_release:
	BUILD="release" make clean test

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
	docker run $(DOCKER_FLAGS) --rm --entrypoint make natalie test_release

cloc:
	cloc include lib src test

ctags:
	ctags -R --exclude=.cquery_cache --exclude=ext --append=no .

format:
	find include -type f -name '*.hpp' -exec clang-format -i --style=file {} +
	find src -type f -name '*.cpp' -exec clang-format -i --style=file {} +
