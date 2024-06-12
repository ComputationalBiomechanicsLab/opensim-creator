# `opensim-creator` Source Code Layout

OpenSim Creator is split into several independent components. The intent of
each is:

| Directory | Description | Depends on |
| - | - | - |
| `OpenSimCreator/` | Implements the OpenSim Creator UI by integrating [OpenSim](https://github.com/opensim-org/opensim-core) against `oscar` | `oscar`, `opensim-core` |
| `OpenSimThirdPartyPlugins` | Copy+paste of third-party OpenSim plugins | `opensim-core` |
| `oscar/` | OpenSim-independent framework for creating scientific tooling UIs | `OpenGL`, `glew`, `SDL2`, `nativefiledialog`, `imgui`, `IconFontCppHeaders`, `ImGuizmo`, `implot`, `stb`, `lunasvg`, `tomlplusplus`, `unordered_dense` |
| `oscar_compiler_configuration/` | Compiler configuration options for `oscar` and `OpenSimCreator` | N/A |
| `oscar_demos/` | Demos that uses the `oscar` API to provide something interesting/useful | `oscar` |
| `oscar_learnopengl/` | Implements https://learnopengl.com/ in terms of the `oscar` API | `oscar` |
