# `third_party/`: Source Code for OpenSim Creator's Third-Party Dependencies

This directory contains the source code for of OpenSim Creator's third-party runtime
dependencies, such that the entire technology stack can be compiled from source on
all target platforms with only a C++ compiler.

You might notice that `OpenSim`/`Simbody` aren't here. That's because they are located
in `osim/` which treats them as an "inline dependency", because OpenSim Creator is also
designed to make developing OpenSim/Simbody easier (you can edit their sourcecode and
recompile+boot OpenSim Creator from scratch in a single build command - this is *very*
useful for debugging/patching upstream).

This directory also operates as a `cmake` "superbuild". It contains a `CMakeLists.txt`
file that uses the `ExternalProject` API to build+install these third-party
dependencies into a standalone directory. That directory can then be used by 
OpenSim Creator's main build via the `CMAKE_PREFIX_PATH` variable, because OpenSim Creator
uniformly uses `find_package` to find all external dependencies.

Because OpenSim Creator uniformly uses `find_package`, you can also reconfigure what
third-party dependencies it *actually* uses. This is particularly useful for LAPACK/BLAS,
because Linux/MacOS provide these at an OS-level, so you can skip building it in this
superbuild and just let `find_package` find the OS-level version.

(yes, it's insane that everything--including OpenBLAS--is built from source, but I *really*
hate managing/debugging/recompiling binary dependencies with various flags like libASAN, specific
architectures, etc., so sue me.)

