# pynogil

Python With No-GIL (Global Interpreter Lock)


## Build

Init submodules:
```
git submodule init
git submodule update
```

Manually build `libuv`:
```
cd libuv
./autogen.sh
./configure
make
cd ..
```

Manually build `duktape`.
On ArchLinux: `sudo pacman -S python2-virtualenv python2-pip`.

```
cd duktape
virtualenv2 venv
source venv/bin/activate
pip install pyyaml
python util/dist.py
cd ..
```

Build `pynogil`:
```
make clean
make
```


## Run

```
./pynogil example1.py

# or
clean; make clean; make; ./pynogil example1.py
```
