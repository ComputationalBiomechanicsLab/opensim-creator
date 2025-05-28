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

Third-party libraries are maintained/changed using `git subtree`:

```bash
git subtree pull --squash --prefix=third_party/googletest https://github.com/google/googletest
```

### NOTE: Some Subtrees are Patched

| Subtree                          | Patched Repo                                                    | Patched Branch    |
| -------------------------------- | --------------------------------------------------------------- | ----------------- |
| `third_party/osim/opensim-core/` | https://github.com/ComputationalBiomechanicsLab/opensim-creator | `opensim-creator` |
| `third_party/imgui/`             | https://github.com/ComputationalBiomechanicsLab/imgui.git       | `opensim-creator` |

Those branches usually contain a couple of commits that are applied on top of upstream
in order fix something quite minor or add a little functionality.
