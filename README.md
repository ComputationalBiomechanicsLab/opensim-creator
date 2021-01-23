![logo with title](resources/logo_with_title.svg)

Lightweight UI for OpenSim

**Unstable WIP: do not use**

`osmv` is a lightweight OpenSim model viewer, coded in C++20, that is
only externally dependent on
[opensim-core](https://github.com/opensim-org/opensim-core) (v4.1) and
OpenGL.


# Building

## Newer Linux's (/w gcc >= 8, CMake >= 3.5)

```bash
git clone https://github.com/adamkewley/osmv.git
mkdir build/
cd build/

cmake ../osmv -DCMAKE_PREFIX_PATH=/path/to/opensim/install
cmake --build . --target osmv

# if you have to use a not-main-system compiler, like `gcc-9`
# from apt:
#
# CC=gcc-9 CXX=g++-9 cmake ../osmv -DCMAKE_PREFIX_PATH=/path/to/opensim/install
# cmake --build . --target osmv

# run: ./osmv
```

## Windows

For Windows, you will need:

- Visual Studio 2017 or later ([download](https://visualstudio.microsoft.com/downloads/))
- CMake >= 3.13 ([download](https://cmake.org/download/))
- An OpenSim4.1+ install (e.g. from [here](https://simtk.org/frs/?group_id=91))
- Or to have built your own OpenSim install (see project [README](https://github.com/opensim-org/opensim-core/)
- (if you want to build self-extracting installers): have NSIS ([download](https://nsis.sourceforge.io/Download))

### On the Command-Line

```batch
git clone https://github.com/adamkewley/osmv.git
mkdir build
cd build/

REM set -DCMAKE_PREFIX_PATH to your OpenSim install if it can't
REM find OpenSim
REM
REM Generator (-G) list:
REM     - https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html

cmake ../osmv -G "Visual Studio 15 2017" -A "x64"
cmake --build . --target osmv

REM run: .\osmv
```

### In CMake + Visual Studio GUI

- Download `osmv` from this repository
- Open the CMake GUI
- Select the `osmv\` directory as the source directory
- Select a build directory (your choice)
- Click `Configure`
- Make sure CMake suggests the correct generator (e.g. Visual Studio 2017) and build type (64-bit)
- If it can't find OpenSim (e.g. because you built it yourself), set `CMAKE_PREFIX_PATH` to your
  `OpenSim` install (e.g. `C:\Users\adam\Desktop\opensim-install\`) and try to configure again
- It should configure
- Click `Generate`
- Click `Open Project`
- Set `osmv` as the active project (right click it `set as active project`)
- CTRL+F5 (or Build -> Run) should boot `osmv`
