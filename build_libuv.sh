#!/bin/bash
cd libuv
make clean
./autogen.sh
./configure
make