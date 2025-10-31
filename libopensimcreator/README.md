# `libopensimcreator`: The library behind the OpenSim Creator UI

`libopensimcreator` combines `liboscar` (the UI engine) with `libosim`
(OpenSim + Simbody bindings) to create a UI for OpenSim that can be
launched by executables (e.g. `osc`).

| Directory | Description |
| - | - |
| `ComponentRegistry/` | Support for OSC-specific registries that contain `OpenSim::Component`s with additional OSC-specific filtering, metadata, etc. |
| `Documents/` | Support for "document" or "model" classes that are externally manipulated by the UI |
| `Graphics/` | Support for 3D graphics related to `SimTK`/`OpenSim`/`OpenSimCreator` datastructures |
| `Platform/` | Application-wide systems (e.g. `OpenSimCreatorApp`) that may be used by various parts of the UI |
| `Shims/` | Backwards-compatibility shims for features that will probably be available in newer C++ standard libraries |
| `testing/` | Code related to test suites |
| `UI/` | Code for rendering OpenSimCreator's 2D UI |
| `Utils/` | Lightweight utility code that doesn't depend on any of the above |
