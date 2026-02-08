![OPynSim Logo](docs/source/_static/opynsim_logo.svg)

# OPynSim

> Python-Native Musculoskeletal Modelling

> [!CAUTION]
> This is currently **ALPHA** software. You can (of course) use it, but major architectural
> pieces are still being moved around.

- 📚 Looking for the documentation? It's hosted at [docs.opynsim.eu](https://docs.opynsim.eu)


# Overview

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
> This is currently **PRE-ALPHA** software. These examples should be seen as living
> documents that I'm sharing for the sake of discussion, not a working API.

```python
import opynsim
import opynsim.ui
from pathlib import Path
import logging

########################################################################
# Initialization
########################################################################

# Globally set `opynsim`'s log using a Python `logging.LEVEL`
opynsim.set_logging_level(logging.DEBUG)

# Globally set where `opynsim` should look for mesh files that cannot
# be found next to a model file.
#
# `opynsim` supports both `pathlib.Path` and `str` for paths.
opynsim.add_geometry_directory(Path("/path/to/geometry/"))


########################################################################
# Modelling
########################################################################

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

# The physics system of a `Model` can be used to realize a `ModelState`
# to a later computational stage (e.g. compute accelerations from
# dynamics).
#
# Different systems require `ModelState`s at different stages. E.g. a
# visualizer usually wants everything, so `REPORT` (the final stage)
# is the safest bet.
model.realize(state, opynsim.ModelStateStage.REPORT)

########################################################################
# Rendering and Visualization
########################################################################
#
# - `opynsim.graphics` provides 3D rendering capabilities
# - `opynsim.ui`       provides window creation, 2D UI, and visualization capabilities
#
# Both modules require that an `opynsim.App()` has been initialized, because
# graphics APIs (e.g. OpenGL) require initialization before use.

# Create an `App`, which initializes application-wide concerns (e.g. window
# creation, OpenGL initialization).
app = opynsim.App()

# Show a "hello world"-style GUI demo.
#
# This script is blocked until the visualizer is closed.
app.show(opynsim.ui.HelloUI())

# Show a model + state in a standard model viewer.
#
# This script is blocked until the visualizer is closed.
app.show(opynsim.ModelViewer(model, state))

# Create a custom visualizer.
#
# `opynsim` provides Python-native bindings to a 2D UI, 3D renderer,
# and window creation system, ported from OpenSim Creator. This enables
# writing basic visualizers from scratch - useful for model building,
# debugging, and presentations.
class CustomModelVisualizer(opynsim.ui.CustomVisualizer):

    # Initializes data that's retained between frames.
    def __init__(self, model, state):
        self.model = model
        self.state = state
        self.scene_cache = opynsim.graphics.SceneCache()
        self.renderer = opynsim.graphics.SceneRenderer()
        self.renderer_parameters = opynsim.graphics.SceneRendererParameters()
        self.decorations = opynsim.graphics.SceneDecorations()

    # Called by `opynsim.App` when it wants to draw a new frame.
    def on_draw(self):

        # Clear previous frame's decorations
        self.decorations.clear()

        # Generate new decorations for the given model+state and write
        # them to `self.decorations`.
        opynsim.graphics.generate_decorations(
            self.model,
            self.state,
            self.decorations,
            scene_cache=self.scene_cache,
            decoration_options=self.decoration_options
        )

        # Add additional decorations.
        self.decorations.add_arrow(
            begin=np.array([0.0, 1.0, 0.0]),
            end=np.array([0.0, 2.0, 0.0]),
        )

        # Setup rendering parameters.
        #
        # A renderer takes a sequence of 3D decorations (`SceneDecorations`) and
        # turns them into a 2D image. `opynsim`'s rendering conventions closely
        # follow how game engines handle rendering (e.g. search/LLM for "what is a
        # view matrix in 3D rendering?").
        self.renderer_parameters.view_matrix = opynsim.maths.look_at(
            from=np.array([1.0, 0.0, 1.0]),
            to=np.array([0.0, 3.0, 0.0]),
            up=np.array([0.0, 1.0, 0.0])
        )
        self.renderer_parameters.projection_matrix = opynsim.maths.perspective(
            vertical_fov=180.0,
            aspect_ratio=self.aspect_ratio(),
            z_near=0.1,
            z_far=1.0
        )
        self.renderer_parameters.dimensions = self.dimensions()
        self.renderer_parameters.pixel_density = self.pixel_density()  # HiDPI

        # Render the scene to a 2D `RenderTexture` on the GPU.
        render_texture = self.renderer.render(decorations, self.renderer_parameters)

        # Blit (copy) the 2D `RenderTexture` to the visualizer to present it to
        # the user.
        self.blit_to_visualizer(render_texture)

# Create a visualizer.
visualizer = CustomModelVisualizer(model, state)

# Continually show the visualizer until it's closed.
app.show(visualizer)

# OPynSim also packages a complete visualization and UI framework, ported
# from OpenSim Creator.
opynsim.ui.view_model_in_state(model, state)
```

# TODO

- Sanity-check MSVC runtime in opynsim:
>OPynSim: If your extension links dynamically to:
>
> - `vcruntime140.dll`
> - `msvcp140.dll`
> - The UCRT (Universal C Runtime)
>
> Then you **cannot assume those are present on every Windows 11 machine**
- Sanity-check MacOS SDK/ABI in opynsim
- Python declarations `.pyd` for IDEs etc.
- API documentation in the documentation build
- Developer guide that specifically focuses on CLion
- READMEs/architectural explanations
- Establish standard Python-style pretty-printing for any binding-exposed classes
