#!/usr/bin/env python3

import opynsim                  # This works with the local virtual environment because `scripts/setup_venv.py` uses `pip install -e ./opynsim/`
import opynsim._opynsim_native  # This is the _private_ native module, but can be handy to access when debugging

# Put code here that you'd like to run via your IDE's native debugger and
# then run the `opynsim_debugger.exe` target, which will build it, boot it,
# and attach the native debugger to it.
#
# (alternatively, run this script under a Python debugger via the project's virtual
#  environment)
print(f"opynsim path = {opynsim.__file__}, native path = {opynsim._opynsim_native}")
print("Hello, world!")
