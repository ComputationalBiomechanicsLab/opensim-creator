# `third_party/`: Source Code for OpenSim Creator's Third-Party Dependencies

This directory contains the source code for of OpenSim Creator's third-party runtime
dependencies, such that its entire dependency tree can be compiled from source on
all target platforms with only `cmake` and a C++ compiler.

There's a `CMakeLists.txt` file that uses the `ExternalProject` API to build+install
these third-party dependencies into a standalone install directory. That directory
can then be used by OpenSim Creator's main build via the `CMAKE_PREFIX_PATH` variable.

You don't strictly *need* to build everything from source: the main `opensim-creator`
build uses `find_package` to find each upstream dependency, so if you know your system
provides all of the dependencies already you can probably (parts of) this.


## Update Procedure

Third-party libraries are maintained/changed using `git subtree`, for example:

```bash
git subtree pull --prefix=third_party/opynsim/ https://github.com/opynsim/opynsim.git  main
git subtree pull --prefix=third_party/oscar/   https://github.com/adamkewley/oscar.git main
```

You can list all subtrees with:

```bash
for d in $(find third_party/ -mindepth 1 -maxdepth 1 -type d); do v=$(git log --grep "git-subtree-dir: $d" -1 --pretty=format:"%b" | grep git-subtree-split); if [ -n "$v" ]; then echo "$d $v"; fi; done
```
