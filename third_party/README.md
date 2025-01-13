# `third_party/`: Source Code for OpenSim Creator's Third-Party Dependencies

This directory contains the source code for all (recursive) of OpenSim Creator's
third-party runtime dependencies. `git submodule` is used fetch/version OpenSim-related
dependencies, where reasonable, because OpenSim is changed/patched more often than
other libraries in the OpenSim Creator project.

This directory also operates as a `cmake` "superbuild". It contains a `CMakeLists.txt`
file that uses the `ExternalProject` API to build+install these third-party
dependencies into a standalone directory that can be used by OpenSim Creator's
main build via `CMAKE_PREFIX_PATH`, because OpenSim Creator uses `find_package`
for any not-vendored (i.e. modified) code.

Because of the separation between the third-party build and OpenSim Creator
via `find_package`, you don't *have* to build all dependencies from source - as
long as OpenSim Creator's main build can find the necessary libraries, it doesn't
matter where they came from (or whether they were built from here). This is
useful to know if you plan on (e.g.) using an OS-provided LAPACK, or want to
build against a different version of `osim`, etc.

**note**: developers should update `README.osc` with the corresponding upstream
version when not using `git submodule`s.
