.PHONY: all docs install build configure mkbuild clean

all: build

docs:
	rm -rf docs
	doxygen

install: configure
	cmake --build build --target install --config Release

build: configure
	cmake --build build --config Release

configure: mkbuild
	cmake -DCMAKE_BUILD_TYPE:STRING=Release -S . -B build

mkbuild:
	mkdir -p build

clean:
	rm -rf build
