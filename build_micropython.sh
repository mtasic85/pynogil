#!/bin/sh
cp misc/micropython_javascript_Makefile micropython/ports/javascript/
cd micropython/ports/javascript
make clean
make -f micropython_javascript_Makefile
