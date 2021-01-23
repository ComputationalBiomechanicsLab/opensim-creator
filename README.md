![logo with title](resources/logo_with_title.svg)

A lightweight development UI for [OpenSim](https://github.com/opensim-org/opensim-core)

`osmv` is a lightweight GUI for the OpenSim C++ API. It is designed to be easy to build and only depends on OpenSim and a few easy-to-aquire GUI libraries (SDL2, GLEW, and ImGui). `osmv` was developed to make it easy to create prototype GUIs on top of the OpenSim API. It is not designed as a production-ready user-facing GUI for OpenSim. For that, you should use the official [OpenSim GUI](https://github.com/opensim-org/opensim-gui).


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
