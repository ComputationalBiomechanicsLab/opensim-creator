# `OpenSimCreator`

> Implementation code for the OpenSim Creator UI

This project directly integrates the [OpenSim API](https://github.com/opensim-org/opensim-core)
against the `oscar` API.

The general structure of this project is work-in-progress, primarly because
I'm midway through refactoring `oscar` into a more sane layout.

The implementation does not provide a `main` function (i.e. it's not an executable,
but a library). See `opensim-creator/apps` for applications that use the library API
to boot something.

| Directory | Description |
| - | - |
| `ComponentRegistry/` | Support for OSC-specific registries that contain `OpenSim::Component`s with additional OSC-specific filtering, metadata, etc. |
| `Documents/` | Support for "document" or "model" classes that are externally manipulated by the UI |
| `Graphics/` | Support for 3D graphics related to `SimTK`/`OpenSim`/`OpenSimCreator` datastructures |
| `Platform/` | Application-wide systems (e.g. `OpenSimCreatorApp`) that may be used by various parts of the UI |
| `UI/` | Code for rendering OpenSimCreator's 2D UI (primarily, via [Dear ImGui](https://github.com/ocornut/imgui)) |
| `Utils/` | Lightweight utility code that doesn't depend on any of the above |
