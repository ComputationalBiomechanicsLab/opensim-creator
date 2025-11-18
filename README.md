# Oscar

> Create research tools in pure C++

> [!CAUTION]
> This isn't intended for production use in anything but @adamkewley's projects. You can use
> it, of course, but don't expect stability or support.
>
> At the moment, `oscar` is effectively an extraction of the engine code of
> [opensim-creator](https://www.opensimcreator.com), so that new tools can be built independently
> of that project. My intention is that this project will grow into its own thing (i.e. gain its
> own documentation pages, used in more than one project, etc.) but it will take some time for
> that to happen.

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
