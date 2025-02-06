# `third_party/`: Source Code for OpenSim Creator's Third-Party Dependencies

This directory contains the source code for of OpenSim Creator's third-party runtime
dependencies, such that the entire technology stack can be compiled from source on
all target platforms with only `cmake` and a C++ compiler and no internet connection.

This directory operates as a `cmake` "superbuild". It contains a `CMakeLists.txt`
file that uses the `ExternalProject` API to build+install these third-party
dependencies into a standalone install directory. That directory can then be used by 
OpenSim Creator's main build via the `CMAKE_PREFIX_PATH` variable.

Because OpenSim Creator uniformly uses `find_package`, you can also reconfigure what
third-party dependencies it *actually* uses at configure-time. This is particularly
useful for LAPACK/BLAS, because Linux/MacOS provide these at an OS-level, so you can
skip building it in this superbuild and just let `find_package` find the OS-level
version.

## Note: `opensim-core`/`simbody` Get Special Treatment

OpenSim Creator tries to use `opensim-core` and `simbody` without any modifications
but, sometimes, there might be additional patches. For this reason, the `opensim-core`
and `simbody` seen here are usually pulled from a branch on `ComputationalBiomechanicsLab`:

```bash
git subtree pull --squash --prefix=third_party/opensim-core https://github.com/ComputationalBiomechanicsLab/opensim-core opensim-creator
git subtree pull --squash --prefix=third_party/simbody https://github.com/ComputationalBiomechanicsLab/simbody opensim-creator
```

Also, `opensim-core` and `simbody` aren't built by the "superbuild". They're built using
a custom cmake build in `libosim`. This is because OSC doesn't use all parts of OpenSim
(notably, Moco), and it statically compiles all libraries. Doing it this way means that
developers can (e.g.) edit OpenSim's sources when debugging and recompile the whole
stack including the OpenSim Creator GUI in one step - very very very useful for finding+fixing
bugs in those upstream projects.

