![workflow](https://github.com/ComputationalBiomechanicsLab/opensim-creator/actions/workflows/continuous-integration-workflow.yml/badge.svg)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.7703588.svg)](https://doi.org/10.5281/zenodo.7703588)

# OpenSim Creator <img src="resources/textures/logo.svg" align="right" alt="OpenSim Creator Logo" width="128" height="128" />

> A UI for building OpenSim models

- 📥 Want to download it? [Download the latest release here](../../releases/latest)
- 🚀 Want to install it? [It's explained in the documentation](https://docs.opensimcreator.com)
- 📚 Want to learn the basics? [View the documentation online](https://docs.opensimcreator.com)
- 📖 Want to cite the project? [See the citation section of this README](#citing)
- 🧬 Want to know more about the project? [See www.opensimcreator.com](https://www.opensimcreator.com)
- ❓ Have a question? [Go to the discussions page](../../discussions)
- 🐛 Found a bug or want to request a feature? [Post it on the issues page](../../issues)
- 🏗️ Want to build it from source? [There's a development section in the documentation](https://docs.opensimcreator.com)


## 👓 Overview

![screenshot](docs/source/_static/screenshot.png)

OpenSim Creator (`osc`) is a standalone UI for building and editing
[OpenSim](https://github.com/opensim-org/opensim-core) models. It's available
as a freestanding all-in-one [installer](../../releases/latest) for Windows 10,
MacOS Ventura, and Ubuntu 20 (or newer versions).

`osc` started development in 2021 in the [Biomechanical Engineering](https://www.tudelft.nl/3me/over/afdelingen/biomechanical-engineering)
department at [TU Delft](https://www.tudelft.nl/). Architecturally, `osc` is a C++ codebase
that is directly integrated against the [OpenSim core C++ API](https://github.com/opensim-org/opensim-core). It
otherwise only uses lightweight open-source libraries that can easily be built from source
(e.g. [SDL](https://www.libsdl.org/), [ImGui](https://github.com/ocornut/imgui), and [stb](https://github.com/nothings/stb))
to implement the UI on all target platforms. This makes `osc` fairly easy to build,
integrate, and package.


<a name="citing"></a>
## 📖 Citing/Acknowledging

OpenSim Creator doesn't have a central _written_ software publication that you can cite (yet 😉). However, if you _need_ to directly cite OpenSim Creator (e.g. because you think it's relevant that you built a model with it), the closest thing you can use is our DOI-ed Zenodo releases (metadata available in this repo: `CITATION.cff`/`codemeta.json`):

> Kewley, A., Beesel, J., & Seth, A. (2024). OpenSim Creator (0.5.16). Zenodo. https://doi.org/10.5281/zenodo.14039957

If you need a general citation for the simulation/modelling technique, you can directly cite OpenSim via this paper:

> Seth A, Hicks JL, Uchida TK, Habib A, Dembia CL, et al. (2018) **OpenSim: Simulating musculoskeletal dynamics and neuromuscular control to study human and animal movement.** _PLOS Computational Biology_ 14(7): e1006223. https://doi.org/10.1371/journal.pcbi.1006223


# ❤️ Acknowledgements

We would like to thank the [Chan Zuckerberg Initiative](https://chanzuckerberg.com/) which
currently funds OpenSim Creator's development through the "Essential Open Source Software
for Science" grant scheme (Chan Zuckerberg Initiative DAF, 2020-218896 (5022)).

We would also like to thank the [Department of Biomechanical Engineering at TU Delft](https://www.tudelft.nl/3me/over/afdelingen/biomechanical-engineering),
which has provided the necessary institutional support required to keep OpenSim Creator's
development supported and stable.

<table align="center">
  <tr>
    <td colspan="2" align="center">Project Sponsors</td>
  </tr>
  <tr>
    <td align="center">
      <a href="https://www.tudelft.nl/3me/over/afdelingen/biomechanical-engineering">
        <img src="resources/textures/tudelft_logo.svg" alt="TUD logo" height="128" />
        <br />
        Biomechanical Engineering at TU Delft
      </a>
    </td>
    <td align="center">
      <a href="https://chanzuckerberg.com/">
        <img src="resources/textures/chanzuckerberg_logo.svg" alt="CZI logo" width="128" height="128" />
        <br />
        Chan Zuckerberg Initiative
      </a>
    </td>
  </tr>
</table>

We'd also like to thank the wider open-source community. OpenSim Creator wouldn't be
possible without access to high-quality open-source libraries and technical literature
from thousands of contributors.
