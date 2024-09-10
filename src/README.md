# `src/`: OpenSim Creator's Source Code (Libraries)

This directory containing the core source code of OpenSimCreator, which is exposed
as (cmake-based) library targets that the downstream targets (e.g. the `osc` binary)
link to.

OpenSim Creator is split into several independent library components. The intent
of each is:

| Directory | Description | Depends on |
| - | - | - |
| `OpenSimCreator/` | Implements the OpenSim Creator UI by integrating [OpenSim](https://github.com/opensim-org/opensim-core) against `oscar`. Also includes the demo/OpenGL tabs etc. for testing/verification. | `OpenSimThirdPartyPlugins`, `oscar`, `oscar_bookofshaders`, `oscar_demos`, `oscar_learnopengl`, `oscar_simbody`, `opensim-core` |
| `OpenSimThirdPartyPlugins` | Copy+paste of third-party OpenSim plugins | `opensim-core` |
| `oscar/` | OpenSim-independent framework for creating scientific tooling UIs | `OpenGL`, `glew`, `SDL2`, `nativefiledialog`, `imgui`, `IconFontCppHeaders`, `ImGuizmo`, `implot`, `stb`, `lunasvg`, `tomlplusplus`, `unordered_dense` |
| `oscar_bookofshaders/` | Implements https://thebookofshaders.com/ in terms of the `oscar` API | `oscar` |
| `oscar_compiler_configuration/` | Compiler configuration options for `oscar` and `OpenSimCreator` | (nothing) |
| `oscar_demos/` | Demos that uses the `oscar` API to provide something interesting/useful | `oscar` |
| `oscar_learnopengl/` | Implements https://learnopengl.com/ in terms of the `oscar` API | `oscar` |
| `oscar_simbody` | OpenSim-independent bindings between [simbody](https://github.com/simbody/simbody) | `oscar` |
