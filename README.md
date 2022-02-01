# OpenSim Creator <img src="resources/logo.png" align="right" alt="OpenSim Creator Logo" width="128" height="128" />

> A thin UI for building OpenSim models

📥 Download the latest release [here](../../releases/latest)

![screenshot](docs/source/_static/screenshot.png)

OpenSim Creator (`osc`) is a standalone UI for building
[OpenSim](https://github.com/opensim-org/opensim-core) models. It is
designed as a proof-of-concept GUI with the intent that some of its
features may be merged into the official [OpenSim GUI](https://github.com/opensim-org/opensim-gui).

Architectrually, `osc` mostly uses C/C++ that is directly integrated
against the [OpenSim core API](https://github.com/opensim-org/opensim-core) and otherwise only
uses lightweight open-source libraries (e.g. SDL2, GLEW, and ImGui) that can be built from source
on all target platforms. This makes `osc` fairly easy to build, integrate, and package.

`osc` started development in 2021 in the [Biomechanical Engineering](https://www.tudelft.nl/3me/over/afdelingen/biomechanical-engineering)
department at [TU Delft](https://www.tudelft.nl/). It is funded by the
Chan Zuckerberg Initiative's "Essential Open Source Software for
Science" grant (Chan Zuckerberg Initiative DAF, 2020-218896 (5022)).

<table align="center">
  <tr>
    <td colspan="2" align="center">Project Sponsors</td>
  </tr>
  <tr>
    <td align="center">
      <a href="https://www.tudelft.nl/3me/over/afdelingen/biomechanical-engineering">
        <img src="resources/tud_logo.png" alt="TUD logo" width="128" height="128" />
        <br />
        Biomechanical Engineering at TU Delft
      </a>
    </td>
    <td align="center">
      <a href="https://chanzuckerberg.com/">
        <img src="resources/chanzuckerberg_logo.png" alt="CZI logo" width="128" height="128" />
        <br />
        Chan Zuckerberg Initiative
      </a>
    </td>
  </tr>
</table>


# 🚀 Installation

> 🚧 **ALPHA-STAGE SOFTWARE** 🚧: OpenSim Creator is currently in development, so
> things are prone to breaking. If a release doesn't work for you,
> report it on the [issues](../../issues)
> page, try a different [📥 release](../../releases)
> or try downloading the latest passing build from [⚡ the actions page](../../actions)
> (requires being logged into GitHub)

You can download a release from the [📥 releases](../../releases) page. The latest
release is [📥 here](../../releases/latest). Also, OpenSim Creator is regularly built
from source using GitHub Actions, so if you want a bleeding-edge--but unreleased--build
of OpenSim Creator check [⚡ the actions page](../../actions) (downloading a CI build
requires being logged into GitHub; otherwise, you won't see download links).

The release process builds installers that work slightly differently on each platform:

- **Windows Installation**: Download a release, unzip it (if necessary), run the `.exe`
  self installer, follow the usual `next`, `next`, `finish` wizard. Run `OpenSimCreator` by typing
  `OpenSimCreator` in your start menu, or browse to `C:\Program Files\OpenSimCreator\`

- **Mac Installation**: Download a release, unzip it (if necessary). Double click the `dmg`
  file to mount it, drag `osc` into your `Applications` directory. Browse to the `Applications`
  directory in `Finder`, right-click the `osc` application, click `open`, continue past any
  security warnings. After running it the first time, you can boot it as normal (e.g. `Super+Space`,
  `osc`, `Enter`)

- **Debian/Ubuntu Installation**: Download a release, unzip it (if necessary). Double-click the
  `.deb` package and install it through your package manager UI. Alternatively, you can install it
  through the commandline: `apt-get install -yf ./osc-X.X.X_amd64.deb` (or similar). Once installed,
  the `osc` or `OpenSim Creator` shortcuts should be available from your desktop, or you can browse
  to `/opt/osc`


# 🏗️  Building

> Because it's still early-stage, there are no full-fat build instructions available (yet).
> However, the build is relatively simple and typically runs end-to-end on a standard C++
> development machine (e.g. one with Visual Studio and CMake installed). The 
> [GitHub action](.github/workflows/continuous-integration-workflow.yml) that builds OSC
> essentially just runs scripts in the [scripts/](scripts/) folder.

The build scripts below should work on a standard C++ developer's machine (as long as you have 
a C/C++ compiler, CMake, etc. installed):

| OS | ⚙️ Build Script | Usage Example |
| :-: | :-: | :-: |
| Windows | [.bat](scripts/build_windows.bat) | `git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator && cd opensim-creator && scripts\build_windows.bat` |
| Mac | [.sh](scripts/build_mac-catalina.sh) | `git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator && cd opensim-creator && scripts/build_mac-catalina.sh` |
| Ubuntu/Debian | [.sh](scripts/build_debian-buster.sh) | `git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator && cd opensim-creator && scripts/build_debian-buster.sh` |
