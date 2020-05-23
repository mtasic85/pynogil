#!/bin/sh
cd duktape
rm -r venv dist
virtualenv2 venv
source venv/bin/activate
pip install pyyaml
python util/dist.py