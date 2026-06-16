# `docker/`: Reproducible Build Environments

OpenSim Creator's release build uses `almalinux8.Dockerfile` to build/test/debug
a release. This produces binaries that have maximal compatibility with a variety
of Linux distributions.

`ubuntu24.Dockerfile` is also provided because it roughly reflects a
typical desktop development environment.

## Example Usage

```bash
# Build + tag an AlmaLinux8 image
cd docker/ && docker build -f almalinux8.Dockerfile -t opensim-creator:almalinux8 . && cd -

# Boot an AlmaLinux8 container that doesn't clobber the
# developer's source directory, `.venv/` directory,
# or `build/` directory.
docker run --rm -it \
    --device /dev/dri:/dev/dri \
    -v ${PWD}:/opensim-creator:ro \
    -v ${PWD}/build-almalinux8:/opensim-creator/build:rw \
    -v ${PWD}/.venv-almalinux8:/opensim-creator/.venv:rw \
    -w /opensim-creator \
    opensim-creator:almalinux8

# Build + tag an Ubuntu24 image
cd docker/ && docker build -f ubuntu24.Dockerfile -t opensim-creator:ubuntu24 . && cd -

# As above, but with Ubuntu24
docker run --rm -it \
    --device /dev/dri:/dev/dri \
    -v ${PWD}:/opensim-creator:ro \
    -v ${PWD}/build-ubuntu24:/opensim-creator/build:rw \
    -v ${PWD}/.venv-ubuntu24:/opensim-creator/.venv:rw \
    -w /opensim-creator \
    opensim-creator:ubuntu24
```

# Platform notes

- Running the test suite without any windowing system: `SDL_VIDEODRIVER=offscreen ctest --preset Development-Inlined -j$(nproc)`
- Running the test suite in a fake Wayland instance: `export XDG_RUNTIME_DIR=$(mktemp -d) ; chmod 700 $XDG_RUNTIME_DIR && SDL_VIDEODRIVER=wayland WLR_BACKENDS=headless cage -- ctest --preset Development-Inlined -j$(nproc)`
- Running the test suite with X11: `SDL_VIDEODRIVER=x11 xvfb-run ctest --preset Development-Inlined -j$(nproc)`
