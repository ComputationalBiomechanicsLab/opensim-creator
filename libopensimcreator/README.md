# `libopensimcreator/`: Implementation Code For OpenSim Creator

`libopensimcreator` combines `liboscar` (the UI engine) with `libopynsim`
(incl. OpenSim + Simbody bindings) to create a UI for OpenSim that can be
launched by executables (e.g. `osc` - the main application executable).

| Directory | Description |
| - | - |
| `cmake/` | CMake-specific scripts |
| `Documents/` | Support for "document" or "model" classes that are externally manipulated by the UI |
| `Platform/` | Application-wide systems (e.g. `OpenSimCreatorApp`) that may be used by various parts of the UI |
| `tests/` | Code related to test suites |
| `UI/` | Code for rendering OpenSimCreator's 2D UI |
