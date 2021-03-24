config = Debug
target = ri

all: build/$(config)/$(target)

run: build/$(config)/$(target)
	@build/$(config)/$(target)

gdb: build/$(config)/$(target)
	@gdb build/$(config)/$(target)

book: build/$(config)/$(target)
	@cmake -E echo "generating book.md ..."
	@build/$(config)/$(target) book

book.epub: book book.md res/book.css res/book.jpg
	@cmake -E echo "generating book.epub ..."
	@cmake -E rm -f book.epub
	@pandoc --css res/book.css --epub-cover-image=res/book.jpg book.md -o book.epub

book.mobi: book.epub
	@cmake -E echo "generating book.mobi ..."
	@cmake -E rm -f book.mobi
	@ebook-convert book.epub book.mobi

format:
	@cmake -P res/format.cmake

clean:
	@cmake -E remove_directory build

vim: build/build.ninja
	@gvim . >/dev/null 2>&1

build/$(config)/$(target): build/build.ninja src/main.hpp src/main.cpp src/book.hpp src/book.cpp
	@cmake --build build --config $(config) --target $(target)

build/build.ninja: CMakeLists.txt
	@cmake --no-warn-unused-cli -G "Ninja Multi-Config" \
	  -DCMAKE_CONFIGURATION_TYPES="Debug;Release;MinSizeRel;RelWithDebInfo" \
	  -DCMAKE_C_STANDARD=11 \
	  -DCMAKE_C_STANDARD_REQUIRED=ON \
	  -DCMAKE_C_EXTENSIONS=OFF \
	  -DCMAKE_C_COMPILER="clang" \
	  -DCMAKE_C_FLAGS_RELEASE="-flto" \
	  -DCMAKE_C_FLAGS_RELWITHDEBINFO="-flto=thin" \
	  -DCMAKE_CXX_STANDARD=20 \
	  -DCMAKE_CXX_STANDARD_REQUIRED=ON \
	  -DCMAKE_CXX_EXTENSIONS=OFF \
	  -DCMAKE_CXX_COMPILER="clang++" \
	  -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
	  -DCMAKE_CXX_FLAGS_RELEASE="-flto" \
	  -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-flto=thin" \
	  -DCMAKE_AR="llvm-ar" \
	  -DCMAKE_NM="llvm-nm" \
	  -DCMAKE_RANLIB="llvm-ranlib" \
	  -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
	  -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld" \
	  -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-Wl,--as-needed -Wl,-s" \
	  -DCMAKE_SHARED_LINKER_FLAGS_RELEASE="-Wl,--as-needed -Wl,-s" \
	  -DCMAKE_EXE_LINKER_FLAGS_MINSIZEREL="-Wl,--as-needed -Wl,-s" \
	  -DCMAKE_SHARED_LINKER_FLAGS_MINSIZEREL="-Wl,--as-needed -Wl,-s" \
	  -DCMAKE_PREFIX_PATH="/opt/boost/lib/boost" \
	  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
	  -DCMAKE_VERBOSE_MAKEFILE=OFF \
	  -B build
