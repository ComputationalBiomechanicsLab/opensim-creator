# `third_party/`: Source Code for OpenSim Creator's Third-Party Dependencies

This directory contains the source code for of OpenSim Creator's third-party runtime
dependencies, such that its entire dependency tree can be compiled from source on
all target platforms with only `cmake` and a C++ compiler.

There's a `CMakeLists.txt` file that uses the `ExternalProject` API to build+install
these third-party dependencies as a "superbuild" into a standalone install directory.
That directory can then be used by OpenSim Creator's main build via the
`CMAKE_PREFIX_PATH` variable (see `CMakePresets.json` in this directory and the main
project's directory).

**Note**: You don't strictly *need* to build everything from source as a superbuild.
Unless prompted otherwise (e.g. via an `INLINED` cache variable), the main
`opensim-creator` build uniformly uses `find_package` to find each of its dependencies,
so if you know your system already supplies the dependencies, or you want to point it
at a different `CMAKE_PREFIX_PATH`, then that can also work.


## Third-Party Library Maintenance

See `NOTICE.txt` in the root repository for upstream URLs and versions. They're located
there because the URL, license, and version are strongly related.

If the third-party code is updated, the `NOTICE.txt` file should also be updated
accordingly.
