.PHONY: all docs install build configure mkbuild clean

all: build

docs:
	rm -rf docs
	doxygen

install:
	cmake -DCMAKE_BUILD_TYPE:String=Release -Dekisocket_BUILD_EXAMPLES:BOOL=OFF -Dekisocket_BUILD_TESTS:BOOL=OFF -S . -B build
	cmake --build build --target install --config Release

build: configure
	cmake --build build --config Release

configure: mkbuild
	cmake -DCMAKE_BUILD_TYPE:STRING=Release -S . -B build

mkbuild:
	mkdir -p build

clean:
	rm -rf build
