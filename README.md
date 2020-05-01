# pynogil

Python With No-GIL (Global Interpreter Lock)

# Build

First, manually build `libuv`:
```
cd libuv
./autogen.sh
./configure
make
```

Second, go back to project root dir:
```
cd ..
```

And build `pynogil`:
```
make clean
make
```
