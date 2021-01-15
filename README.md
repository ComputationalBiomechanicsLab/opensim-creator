# osmv

Lightweight UI for OpenSim

**Unstable WIP: do not use**

`osmv` is a lightweight OpenSim model viewer, coded in C++20, that is
only externally dependent on
[opensim-core](https://github.com/opensim-org/opensim-core) (v4.1) and
OpenGL.


# Building

- `osmv` depends on a fully-built-and-installed OpenSim:

  - **Linux**: this usually means building + installing OpenSim yourself
  - **Windows**: you can download a release of OpenSim from [here](https://simtk.org/frs/?group_id=91)
  - **OSX**: Not tested yet-  you *might* be able to build `osmv` but no guarantees

- `osmv` is written in C++17 and uses the `<filesystem>` standard library. Only modern compilers
  (e.g. g++ >= 8, VS2017) will be able to compile it


## Older Ubuntu's (16/18/Xenial/Bionic, /w `gcc-9` from toolchain test repo)

Older Ubuntu's dont provide all the necessary dependencies via `apt`, so you'll have to build/get
some components the long way. [This](scripts/ubuntu-xenial_e2e-build.sh) script shows how to build
`osmv` on a clean install of Xenial. The script:

- Installs `g++-9`, because `osmv` is made with C++17 and uses `<filesystem>`

- (optional) Builds OpenSim 4.1 from source, because the OpenSim team do not publish formal
  Linux releases (if you don't need this, then I'm assuming you have already built + installed
  OpenSim yourself on Linux)

- Builds `osmv`

You should read [the script](scripts/ubuntu-xenial_e2e-build.sh) for more info.


## Windows

For Windows, you will need:

- Visual Studio 2017 or later ([download](https://visualstudio.microsoft.com/downloads/))
- CMake >= 3.13 ([download](https://cmake.org/download/))
- An OpenSim4.1+ install (e.g. from [here](https://simtk.org/frs/?group_id=91))
- Or to have built your own OpenSim install (see project [README](https://github.com/opensim-org/opensim-core/)
- (if you want to build self-extracting installers): have NSIS ([download](https://nsis.sourceforge.io/Download))

### GUI Guide

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

### Command-line Guide

```batch
git clone https://github.com/adamkewley/osmv

mkdir osmv-build
cd osmv-build
cmake ../osmv
cmake --build . --config Release --target osmv
./osmv
```
