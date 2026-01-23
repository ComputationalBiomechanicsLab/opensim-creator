# OPynSim

> Python-Native Musculoskeletal Modelling

> [!CAUTION]
> This is currently **ALPHA** software. You can (of course) use it, but major architectural
> pieces are still being moved around. [opensim-creator](https://www.opensimcreator.com/) already
> uses it, but that's an internal project, so I can fix any rearchitecting issues 😉.
> 
> The top-level architectural layout should be stabilized by mid-2026.

The aim of OPynSim is to provide a python-native API for musculoskeletal modelling that
doesn't compromise on nearly 20 years of research, feature development, and UI development from
[Simbody](https://github.com/simbody/simbody), [OpenSim](https://simtk.org/projects/opensim),
and [OpenSim Creator](https://www.opensimcreator.com/).

# Installation

See supported platforms

```bash
pip install opynsim
```

# Supported Platforms

OPynSim is supported on the following platform combinations:

| Python Version | Operating System                                                             | Processor Architecture  |
|--------------- |------------------------------------------------------------------------------|-------------------------|
| >=3.12         | Windows                                                                      | amd64 (Intel/AMD)       |
| >=3.12         | Linux (manylinux 234: Debian 12+, Ubuntu 21.10+, Fedora 35+, CentOS/RHEL 9+) | amd64 (Intel/AMD)       |
| >=3.12         | MacOS >= 14.5 (Sonoma)                                                       | amd64 (Intel)           |
| >=3.12         | MacOS >= 14.5 (Sonoma)                                                       | arm64 (Apple Silicon)   |

# Example Usage

> [!CAUTION]
>
> This is currently **ALPHA** software. These examples are more like talking points, than a final API.

```python
import opynsim
import opynsim.ui
from pathlib import Path
import logging

# Setup globals (if necessary)
opynsim.set_logging_level(logging.DEBUG)  # Python's `logging.LEVEL` is supported
opynsim.add_geometry_directory(Path("/path/to/geometry/"))  # Python's `pathlib.Path` is supported

# A `ModelSpecification` specifies how OPynSim should build the model.
model_specification = opynsim.import_osim_file("/path/to/model.osim")

# A `ModelSpecification` can be compiled into a `Model`. Compilation
# validates the specification and assembles a physics system from it.
#
# Callers can use a `Model` to ask questions about the physics system
# (What inputs/outputs does it have? What coordinates does it have?) and
# produce/edit `ModelState`s.
model = model_specification.compile()

# A `ModelState` is one state of a `Model`. All `Model`s can produce an
# initial state.
#
# Callers can manipulate `ModelState`s directly (e.g. manipulate state
# vectors "in the raw"), or in tandem with a `Model`.
state = model.initial_state()

# The physics system of a `Model` can be used to update a `ModelState`s
# stage (e.g. to compute accelerations from dynamics).
model.realize(state, opynsim.ModelStateStage.REPORT)

# OPynSim also packages a complete visualization and UI framework, ported
# from OpenSim Creator.
opynsim.ui.view_model_in_state(model, state)
```

# TODO

- Sanity-check MSVC runtime in opynsim
- Sanity-check MacOS SDK/ABI in opynsim
- Python declarations `.pyd` for IDEs etc.
- API documentation in the documentation build
- Developer guide that specifically focuses on CLion
- READMEs/architectural explanations
