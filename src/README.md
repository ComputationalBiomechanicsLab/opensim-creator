# `opensim-creator` Source Code Layout

OpenSim Creator is split into several independent components. The intent of
each is:

| Component | Description |
| - | - |
| `OpenSimCreator/` | Implements the OpenSim Creator UI by integrating [OpenSim](https://github.com/opensim-org/opensim-core) against `oscar` |
| `oscar/` | OpenSim-independent framework for creating scientific tooling UIs |
| `oscar_compiler_configuration/` | Compiler configuration options for `oscar` and `OpenSimCreator` |
| `oscar_demos/` | Demo that use the `oscar` API to provide something interesting/useful |
| `oscar_learnopengl` | Implements https://learnopengl.com/ in terms of the `oscar` API |
