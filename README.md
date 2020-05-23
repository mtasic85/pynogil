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

Manually build `libuv` (GCC/Clang is requirement):
```
cd libuv
./autogen.sh
./configure
make
cd ..
```

Manually build `duktape` (Python is requirement).
On ArchLinux: `sudo pacman -S python2-virtualenv python2-pip`.

```
cd duktape
virtualenv2 venv
source venv/bin/activate
pip install pyyaml
python util/dist.py
cd ..
```

Manually build `micropython` (Emscripten is requirements):
```
cp micropython_javascript_Makefile micropython/ports/javascript/
cd micropython/ports/javascript
make -f micropython_javascript_Makefile
cd ../../..
cp micropython/ports/javascript/build/micropython.js .
```


## Build

Build `pynogil`:
```
make clean
make
```


## Run

```
./pynogil example1.py
```


## Build / Run

You might find this useful if frequently update code and run new one:

```
clean; make clean; make; ./pynogil examples/example0.py
```
