# Reverend Insanity
Downloads Reverend Insanity by Gu Zhen Ren and generates ebook files.

## Dependencies
Install dependencies.

```sh
sudo apt install binutils-dev make ninja-build
sudo apt install clang-11 llvm-11 lld-11 lldb-11 libc++-11-dev libc++abi-11-dev
sudo apt install libssl-dev libutf8proc-dev libxml2-dev
sudo apt install calibre pandoc

sudo mkdir /opt/boost
sudo chown $(id -u):$(id -g) /opt/boost

wget https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.bz2
mkdir boost && tar xf boost_1_75_0.tar.bz2 -C boost --strip-components=1; cd boost

CC=clang-11 CXX=clang++-11 ./bootstrap.sh

cat > config.jam <<'EOF'
using clang-linux : 11.0.0 : /usr/bin/clang++-11 :
<cxxstd> "20"
<cxxflags> "-stdlib=libc++ -flto=full"
<linkflags> "-fuse-ld=lld -Wl,--as-needed -Wl,-s" ;
EOF

./b2 --project-config=config.jam --prefix=/opt/boost \
  --layout=system --build-dir=build-release \
  --without-graph_parallel \
  --without-mpi \
  --without-python \
  architecture=x86 address-model=64 \
  variant=release optimization=space threading=multi \
  link=shared runtime-link=shared debug-symbols=off install
```

## Usage
See [makefile](makefile) for more details.

```sh
7z x res/cache.7z
rm -rf cache/txt; make config=Release book.mobi
```
