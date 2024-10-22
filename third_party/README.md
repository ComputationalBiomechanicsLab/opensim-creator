# `third_party/`: Source Code for OpenSim Creator's Third-Party Dependencies

This directory contains the source code for all (recursive) of OpenSim Creator's
third-party runtime dependencies. We use `git submodule` to fetch/version the
dependencies.

This directory also operates as a `cmake` build. It contains a `CMakeLists.txt`
file that uses the `ExternalProject` API to build+install these third-party
dependencies into a standalone directory, so that OpenSim Creator's main build
(`../CMakeLists.txt`) can find all necessary dependencies via a combination
of `find_package` and `CMAKE_PREFIX_PATH`.

Because of the separation between the third-party build and OpenSim Creator
via `find_package`, you don't *have* to build all dependencies from source - as
long as OpenSim Creator's main build can find the necessary libraries, it doesn't
matter where they came from (or whether they were built from here). This is
useful to know if you plan on (e.g.) using an OS-provided LAPACK, or want to
build against a different version of `osim`, etc.
