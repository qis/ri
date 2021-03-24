# Reverend Insanity
Downloads Reverend Insanity by Gu Zhen Ren and generates ebook files.

## Boost
Install boost.

```sh
sudo mkdir /opt/boost
sudo chown $(id -u):$(id -g) /opt/boost

./bootstrap.sh

cat > config.jam <<'EOF'
using clang-linux : 11.0.0 : /usr/bin/clang++ :
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
