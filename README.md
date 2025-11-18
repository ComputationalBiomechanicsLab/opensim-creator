# `oscar`

> Create research tools in pure C++

**TODO**: This is currently **WIP** and is effectively just the
engine code from [opensim-creator](https://www.opensimcreator.com)
extracted into its own codebase. It's only really usable via `add_subdirectory`.

# Build Instructions (Development)

## Bash Terminal (MacOS/Linux)

```bash
# Or ErrorCheck / RelWithDebInfo / Release
BUILD_TYPE=Development

# Build bundled dependencies
cd third_party && cmake --workflow --preset oscar_third-party_${BUILD_TYPE} ; cd -

# Build
cmake --workflow --preset oscar_${BUILD_TYPE}
```

## Batch Terminal (Windows)

```batch
REM Activate Visual Studio environment (if desired)
call scripts\env_visual-studio.bat

REM Or RelWithDebInfo / Release (ErrorCheck only works on Unixes)
set BUILD_TYPE=Development

REM Build bundled dependencies
cd third_party && cmake --workflow --preset oscar_third-party_%BUILD_TYPE% && cd ..

REM Build
cmake --workflow --preset oscar_%BUILD_TYPE%
```
