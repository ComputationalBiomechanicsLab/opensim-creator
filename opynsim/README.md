# opynsim

> Python-native bindings to `libopynsim`

`opynsim` is a python library that internally uses `nanobind` to create python
bindings to native `libopynsim` code. A key philosophy of `opynsim` is that it
should be easy to install, "feel like python", and focus on being high-level and
easy to learn.

A key difference between `opynsim` and the official `opensim` python module is
that `opynsim` is an explicit python-native implementation, whereas `opensim` is
automatically generated from OpenSim's C++ API using SWIG. This means that
`opynsim`'s python API can diverge from its C++ implementation API, which helps
us keep the python API minimal and teachable, while retaining the underlying
power of `libopynsim`.
