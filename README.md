# Oscar

> Create research tools in pure C++

> [!CAUTION]
> This isn't currently intended for public use (only for internal use by [opensim-creator](https://www.opensimcreator.com/)
> and [opynsim](https://github.com/opynsim/opynsim)). Read/use it at your own
> risk, and don't expect stability of support.

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
