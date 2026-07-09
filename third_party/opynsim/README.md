> [!CAUTION]
> This is currently **PRE-ALPHA** software. You can (of course) use it, but major
> architectural pieces are still being moved around.

<h1 align="center">
    <img src="docs/source/_static/opynsim_banner_horizontal.svg" alt="OPynSim banner" />
</h1>

OPynSim is a python-native API for musculoskeletal modeling that doesn't compromise on nearly
20 years of research and feature development the field.

- **Documentation**: [https://docs.opynsim.eu](https://docs.opynsim.eu)
- **Source code**: [https://github.com/opynsim/opynsim](https://github.com/opynsim/opynsim)

OPynSim provides:

- **A Python-native interface** for building and manipulating musculoskeletal models.
- **An integrated visualization API** that supports real-time 3D rendering.
- **High-performance native bindings**, implemented with [nanobind](https://github.com/wjakob/nanobind).
- **Almost zero runtime dependencies**. The entire C++ stack is built into a single
  native extension module that only depends on common system libraries and `numpy>=1.26.0`.
- **Stable Python ABI implementation**. Each release of OPynSim works on any
  Python version ≥ 3.12 on Windows, macOS, and Linux.
- **Excellent compatibility with OpenSim**. It can import/export `.osim` files
  directly and can be used alongside the existing `opensim` Python API. OPynSim is
  already used in production releases of [OpenSim Creator](https://www.opensimcreator.com), which ships to
  thousands of OpenSim ~~testers~~ users.


## 📖 Citing/Acknowledging

If you want to cite the OPynSim project, cite its proposal (we will publish something later on):

> Kewley, A., & Seth, A. (2026). OPynSim: A python-native library for interoperable biomechanical simulations. Zenodo. https://doi.org/10.5281/zenodo.19493285

If you want to cite a specific version of OPynSim (e.g. for reproducibility), then additionally
cite the unique Zenodo DOI corresponding to that version, for example:

> TODO (pre-alpha, no stable releases yet, sorry!)


## ❤️ Acknowledgements

OPynSim is currently in pre-alpha (concept) development in the [Department of Biomechanical Engineering at TU Delft](https://www.tudelft.nl/3me/over/afdelingen/biomechanical-engineering),
which is providing critical institutional support before the project officially begins.

The OPynSim project is expected to begin around mid-June 2026, with two years of funding
from [Open Science NL](https://www.openscience.nl/) ([announcement](https://www.openscience.nl/en/news/45-projects-strengthen-dutch-open-science-infrastructure)). You
can read the project proposal [here](https://doi.org/10.5281/zenodo.19493285). This (pre-alpha)
version of OPynSim is focused on ensuring that the source files, build system, documentation, etc. are in the right place
before the project starts, so that related projects (e.g. [OpenSim Creator](https://www.opensimcreator.com/)) can already be integrated
with it before the project begins.
