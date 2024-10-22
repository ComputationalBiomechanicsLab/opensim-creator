# `src/`: OpenSim Creator's Source Code (Libraries)

This directory contains the source code for libraries that comprise OpenSimCreator. The
libraries are exposed as cmake targets. Those targets are then used by application/executable
targets (e.g. `osc.exe`, in `opensim-creator/apps/`).

The libraries are split into several targets. The intent of each is:

| Directory | Description | Depends on |
| - | - | - |
| `OpenSimCreator/` | Implements the OpenSim Creator UI by integrating [osim](https://github.com/ComputationalBiomechanicsLab/osim) against `oscar`. Also includes the demo tabs for testing/verification. | `oscar`, `oscar_demos`, `osim` |
| `oscar/` | Core engine for creating scientific tooling UIs | `OpenGL`, `glew`, `SDL2`, `nativefiledialog`, `imgui`, `implot`, `stb`, `lunasvg`, `tomlplusplus`, `unordered_dense` |
| `oscar_compiler_configuration/` | Cmake `INTERFACE` target that configures the compiler options for `oscar` and `OpenSimCreator` | (nothing) |
| `oscar_demos/` | Demos that uses the `oscar` API to provide something interesting/useful, such as implement https://learnopengl.com/ in terms of the `oscar` API | `oscar`, `oscar_compiler_configuration` |
