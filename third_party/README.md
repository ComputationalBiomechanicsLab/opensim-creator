# `third_party/`: Source Code for OpenSim Creator's Third-Party Dependencies

This directory contains the source code for of OpenSim Creator's third-party runtime
dependencies, such that the entire technology stack can be compiled from source on
all target platforms with only `cmake` and a C++ compiler and no internet connection.

There's a `CMakeLists.txt` file that uses the `ExternalProject` API to build+install
these third-party dependencies into a standalone install directory. That directory
can then be used by OpenSim Creator's main build via the `CMAKE_PREFIX_PATH` variable.

Because OpenSim Creator uniformly uses `find_package`, you can also reconfigure what
third-party dependencies it *actually* uses at configure-time. This is particularly
useful for LAPACK/BLAS, because Linux/MacOS provide these at an OS-level, so you can
skip building it in this superbuild and just let `find_package` find the OS-level
version.


## Update Procedure

Third-party libraries are maintained/changed using `git subtree`, for example:

```bash
git subtree pull --squash --prefix=third_party/googletest https://github.com/google/googletest v1.17.0
git subtree pull --prefix=third_party/opynsim/ https://github.com/opynsim/opynsim.git master
```

You can list all subtrees with:

```bash
for d in $(find third_party/ -mindepth 1 -maxdepth 1 -type d); do v=$(git log --grep "git-subtree-dir: $d" -1 --pretty=format:"%b" | grep git-subtree-split); if [ -n "$v" ]; then echo "$d $v"; fi; done
```

**Note**: the build procedure also applies patches so some of the upstream code so
that it works better in OpenSim Creator. See `patches/`.
