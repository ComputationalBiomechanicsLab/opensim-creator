# Oscar <img src="docs/oscar_logo.svg" align="right" alt="OPynSim Logo" width="128" height="128" />

> A C++ framework for research UIs.

> [!CAUTION]
> This isn't currently intended for public use (only for internal use by [opensim-creator](https://www.opensimcreator.com/)
> and [opynsim](https://github.com/opynsim/opynsim)). Read/use it at your own
> risk - don't expect stability or support.

Oscar is a project that ships `liboscar`, a C++ library that integrates:

- Platform abstraction (application context, window creation, logging, event handling).
- Rendering support (e.g. `Shader`, `Material`, `Texture2D`, `Color`).
- Immediate UI support (e.g. `ui::draw_text`, `ui::draw_button`).
- Maths support (e.g. `Vector3`, `Matrix3x3`, `look_at`, `AABB`, `BVH`).
- Common/basic file format support (e.g. `CSV`, `DAE`, `PNG`, `JPEG`).
- General utilities (e.g. `CStringView`, `CopyOnUpdPtr<T>`, `TemporaryDirectory`).

Into a single statically-linkable library that only transitively exposes `osc::` and
`std::` symbols. It can easily be built from source, including dependencies, for `x86_64`/`arm64` architectures
on Linux, macOS, and Windows. This makes it ideal for building larger integrated tools.

Oscar is currently an internal project. It was developed as part of [OpenSim Creator](www.opensimcreator.com)
between 2021-2025 and then separated into a standalone project in 2026 so that it could be
used by [OPynSim](https://github.com/opynsim/opynsim). Long-term, the intention is to improve what's here so
that Oscar can be used in other projects.


# Build Instructions (Development)

## Bash Terminal (MacOS/Linux)

```bash
# Or ErrorCheck / RelWithDebInfo / Release
BUILD_TYPE=Development

# Build bundled dependencies
cd third_party && cmake --workflow --preset "${BUILD_TYPE}" ; cd -

# Build
cmake --workflow --preset "${BUILD_TYPE}"
```

## Batch Terminal (Windows)

```batch
REM Activate Visual Studio environment (if desired)
call scripts\env_visual-studio.bat

REM Or RelWithDebInfo / Release (ErrorCheck only works on Unixes)
set BUILD_TYPE=Development

REM Build bundled dependencies
cd third_party && cmake --workflow --preset "%BUILD_TYPE%" && cd ..

REM Build
cmake --workflow --preset "%BUILD_TYPE%"
```
