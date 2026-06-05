.DEFAULT_GOAL := build

BUILD_TYPE ?= Debug

.PHONY: setup
setup:
	conan install \
	  --build=missing \
	  -s build_type=$(BUILD_TYPE) \
	  .
	cmake \
	  -S . \
	  -B build/ \
	  $$(command -v ninja >/dev/null && printf '%s' '-G Ninja') \
	  -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	  -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake

.PHONY: build
build:
	cmake --build build/

.PHONY: test
test: build
	ctest --test-dir build/ --progress \
	&& build/src/cprime/parse/cprime_parse_file_test . \
	&& cmake --build build/ --target check

.PHONY: update-filetests
update-filetests: build
	build/src/cprime/parse/cprime_parse_file_test -u .

.PHONY: clean
clean:
	rm -rf build/

.PHONY: format-code
format-code:
	find src/ -type f \
	  -name '*.c' -o -name '*.h' -o \
	  -name '*.cpp' -o -name '*.hpp' \
	| xargs -r clang-format -i
