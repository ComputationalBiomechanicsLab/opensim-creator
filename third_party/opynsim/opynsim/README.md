# opynsim

> Python-native bindings to `libopynsim`

`opynsim` is a python package that contains python code (in `opynsim/`) and
native code (source code in `_opynsim_native/`, built to `opynsim/_opynsim_native.X`). The
native code uses `nanobind` to bind Python to `libopynsim`.

The philosophy of `opynsim` is that it must be easy to install, "feel like python" (i.e.
Python-native), and focus on being high-level and easy to learn, which results in
these architectural decisions:

- **Everything is statically linked into one ABI_STABLE native extension module**. The
  native module must have very minimal transitive dependencies (e.g. on glibc, libstdc++)
  so that it can be installed on a wide variety of computers with no further steps.
- **The Python API is explicitly designed for Python, not automatically generated from C++**. This
  is in contrast to `opensim`, which relies on automatic generation. It's harder to write
  bindings this way, but the resulting bindings feel more Python-native because they are
  specifically designed for Python.
