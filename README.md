![workflow](https://github.com/ComputationalBiomechanicsLab/opensim-creator/actions/workflows/continuous-integration-workflow.yml/badge.svg)

# OpenSim Creator <img src="resources/textures/logo.png" align="right" alt="OpenSim Creator Logo" width="128" height="128" />

> A UI for building OpenSim models

[📥 Download the latest release here](../../releases/latest), [▶️ Watch Introduction Videos Here](https://www.youtube.com/playlist?list=PLOPlDtRLhp8c2SWLCQKKd-l4__UainOYk)

![screenshot](docs/source/_static/screenshot.png)

OpenSim Creator (`osc`) is a standalone UI for building and editing
[OpenSim](https://github.com/opensim-org/opensim-core) models. It is available
as a freestanding all-in-one [installer](../../releases/latest) for Windows, Mac, and Linux.

Architecturally, `osc` is a C++ codebase that is directly integrated against
the [OpenSim core C++ API](https://github.com/opensim-org/opensim-core). It otherwise only
uses lightweight open-source libraries that can easily be built from source (e.g. SDL2,
OpenGL, ImGui) to implement the UI on all target platforms. This makes `osc` fairly easy
to build, integrate, and package.

`osc` started development in 2021 in the [Biomechanical Engineering](https://www.tudelft.nl/3me/over/afdelingen/biomechanical-engineering)
department at [TU Delft](https://www.tudelft.nl/). It is currently funded by the
[Chan Zuckerberg Initiative](https://chanzuckerberg.com/)'s "Essential Open Source Software for
Science" grant (Chan Zuckerberg Initiative DAF, 2020-218896 (5022)).

<table align="center">
  <tr>
    <td colspan="2" align="center">Project Sponsors</td>
  </tr>
  <tr>
    <td align="center">
      <a href="https://www.tudelft.nl/3me/over/afdelingen/biomechanical-engineering">
        <img src="resources/textures/tud_logo.png" alt="TUD logo" width="128" height="128" />
        <br />
        Biomechanical Engineering at TU Delft
      </a>
    </td>
    <td align="center">
      <a href="https://chanzuckerberg.com/">
        <img src="resources/textures/chanzuckerberg_logo.png" alt="CZI logo" width="128" height="128" />
        <br />
        Chan Zuckerberg Initiative
      </a>
    </td>
  </tr>
</table>


# 🚀 Installation

You can download a release from the [📥 releases](../../releases) page. The latest
release is [📥 here](../../releases/latest). Also, OpenSim Creator is regularly built
from source using GitHub Actions, so if you want a bleeding-edge--but unreleased--build
of OpenSim Creator check [⚡ the actions page](../../actions) (downloading a CI build
requires being logged into GitHub; otherwise, you won't see download links).

### Windows

- Download a `exe` [release](../../releases)
- Run the `.exe` installer, continue past any security warnings
- Follow the familiar `next`, `next`, `finish` wizard
- Run `OpenSimCreator` by typing `OpenSimCreator` in your start menu, or browse to `C:\Program Files\OpenSimCreator\`.

### Mac

- Download a `dmg` [release](../../releases)
- Double click the `dmg` file to mount it
- Drag `osc` into your `Applications` directory.
- Browse to the `Applications` directory in `Finder`
- Right-click the `osc` application, click `open`, continue past any security warnings to run `osc` for the first time
- After running it the first time, you can boot it as normal (e.g. `Super+Space`, `osc`, `Enter`)

### Debian/Ubuntu

- Download a `deb` [release](../../releases)
- Double-click the `.deb` package and install it through your package manager UI.
- **Alternatively**, you can install it through the command-line: `apt-get install -yf ./osc-X.X.X_amd64.deb` (or similar).
- Once installed, the `osc` or `OpenSim Creator` shortcuts should be available from your desktop, or you can browse
  to `/opt/osc`


# 🏗️  Building

OpenSim Creator is a C++ codebase. It is primarily dependent on a C++ compiler (usually, MSVC on Windows,
clang on Mac, and g++ on Linux) and CMake. It also depends on OpenSim. Other dependencies are included
in-tree (see `third_party/`), either as copied-in libraries or git submodules.

Because everyone's C++ build environment is *slightly* different, there are no catch-all build instructions
available. Instead, we recommend reading + running the automated build scripts, or reading some of the basic
tips-and-tricks for Visual Studio or QtCreator (below).

The following build scripts below should work on a standard C++ developer's machine (as long as you have 
a C/C++ compiler, CMake, etc. installed):

| OS | ⚙️ Build Script | Usage Example |
| :-: | :-: | :-: |
| Windows | [.bat](scripts/build_windows.bat) | `git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator && cd opensim-creator && scripts\build_windows.bat` |
| Mac | [.sh](scripts/build_mac-catalina.sh) | `git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator && cd opensim-creator && scripts/build_mac-catalina.sh` |
| Ubuntu/Debian | [.sh](scripts/build_debian-buster.sh) | `git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator && cd opensim-creator && scripts/build_debian-buster.sh` |


### Visual Studio 2020 Dev Environment Setup

- Run the `bat` [builscript](scripts/build_windows.bat) (described above) to get a complete build.
- In Visual Studio 2020, open `opensim-creator` as a folder project
- Later versions of Visual Studio (i.e. 2017+) should have in-built CMake support that automatically detects that the folder is a CMake project
- Right-click the `CMakeLists.txt` file to edit settings or build the project
- Use the `Switch between solutions and available views` button in the `Solution Explorer` hierarchy tab to switch to the `CMake Targets View`
- Right-click the `osc` CMake target and `Set As Startup Project`, so that pressing `F5` will then build+run `osc.exe`
- (optional): switch the solution explorer view to a `Folder View` after doing this: the CMake view is crap for developing osc
- You should now be able to build+run `osc` from `Visual Studio`
- To run tests, open the `Test Explorer` tab, which should list all of the `googletest` tests in the project

### QtCreator Dev Environment Setup

- Run the appropriate (OS-dependent) buildscript (described above)
- Open QtCreator and then open the `opensim-creator` source directory as a folder
- For selecting a "kit", QtCreator *usually* detects that `osc-build` already exists (side-effect of running the buildscript). You *may* need to "import existing kit/build" and then select `osc-build`, though
- Once QtCreator knows your source dir (`opensim-creator/`) and build/kit (`opensim-creator/osc-build`), it should be good to go


# 🥰 Contributing

If you would like to contribute to OpenSim Creator then thank you 🥰: it's people like you
that make open-source awesome! See [CONTRIBUTING.md](CONTRIBUTING.md) for more details.
