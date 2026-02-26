> [!CAUTION]
> This is currently **PRE-ALPHA** software. You can (of course) use it, but major
> architectural pieces are still being moved around.

<h1 align="center">
    <img src="docs/source/_static/opynsim_banner_horizontal.svg" alt="OPynSim banner" />
</h1>

OPynSim is a python-native API for musculoskeletal modeling that doesn't compromise on nearly
20 years of research, feature development, and UI development from [Simbody](https://github.com/simbody/simbody),
[OpenSim](https://simtk.org/projects/opensim), and [OpenSim Creator](https://www.opensimcreator.com/).

- **Documentation**: [https://docs.opynsim.eu](https://docs.opynsim.eu)
- **Source code**: [https://github.com/opynsim/opynsim](https://docs.opynsim.eu)

OPynSim provides:

- **A Python-native interface** for building and manipulating musculoskeletal models.
- **An integrated visualization API** that supports both 2D widgets (plots, buttons, text) and
  real-time 3D rendering.
- **High-performance native bindings**, implemented with [nanobind](https://github.com/wjakob/nanobind).
- **Almost zero runtime dependencies**. The entire OPynSim C++ stack
  is built into a single native extension module that only depends on common
  system libraries. The Python stack only depends on `numpy` (unpinned).
- **Stable Python ABI implementation**. Each release of OPynSim works on any
  Python version ≥ 3.12 on Windows, macOS, and major Linux distributions.
- **Excellent compatibility with OpenSim**. The OPynSim codebase uses
  a [lightly-patched forks](libosim) of [opensim-core](https://github.com/opensim-orgopensim-core)
  and [simbody](https://github.com/simbody/simbody). The native part is already
  used in production releases of [OpenSim Creator](https://www.opensimcreator.com), which
  ships to thousands of OpenSim ~~testers~~ users.
