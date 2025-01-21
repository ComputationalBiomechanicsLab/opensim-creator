# `libopensimcreator`: Core code for the OpenSim Creator UI

`libopensimcreator` combines `liboscar` (a UI engine) with `libosim`
(OpenSim + Simbody bindings) to create a UI for OpenSim.

| Directory | Description |
| - | - |
| `ComponentRegistry/` | Support for OSC-specific registries that contain `OpenSim::Component`s with additional OSC-specific filtering, metadata, etc. |
| `Documents/` | Support for "document" or "model" classes that are externally manipulated by the UI |
| `Graphics/` | Support for 3D graphics related to `SimTK`/`OpenSim`/`OpenSimCreator` datastructures |
| `Platform/` | Application-wide systems (e.g. `OpenSimCreatorApp`) that may be used by various parts of the UI |
| `UI/` | Code for rendering OpenSimCreator's 2D UI |
| `Utils/` | Lightweight utility code that doesn't depend on any of the above |
| `testing/` | Code related to unit tests |

