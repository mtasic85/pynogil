# pynogil

Python With No-GIL (Global Interpreter Lock)


## Build

First, init submodules:
```
git submodule init
git submodule update
```

Second, manually build `libuv`:
```
cd libuv
./autogen.sh
./configure
make
```

Third, go back to project root dir:
```
cd ..
```

Forth, build `pynogil`:
```
make clean
make
```


## Run

```
./pynogil example1.py
```
