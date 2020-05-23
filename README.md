# pynogil

Python With No-GIL (Global Interpreter Lock)

This is an experiment, a hack, proof-of-concept. Do not use it in production.

*pynogil* is based on excellent async IO event loop library *libuv*, fantastic JavaScript engine *duktape* which comes with compiler and virtual machine, and awesome Python implementation *micropython*.

Real heroes are ones who developed libuv, duktape and micropython. Without you this could not be possible. Thank you!


## Prerequisites

Init submodules:
```
git submodule init
git submodule update
```

Manually build `libuv` (GCC or Clang is requirement):
```
./build_libuv.sh
```

Manually build `duktape` (Python is requirement).
On ArchLinux: `sudo pacman -S python2-virtualenv python2-pip`.

```
./build_duktape.sh
```

Manually build `micropython` (Emscripten is requirement):

```
./build_micropython.sh
```


## Build

Build `pynogil`:
```
make clean
make
```


## Run

Run repl:
```
./pynogil
```

Run file:
```
./pynogil example1.py
```


## Build / Run

You might find this useful if frequently update code and run new one:

```
clean; make clean; make; ./pynogil examples/example0.py
```
