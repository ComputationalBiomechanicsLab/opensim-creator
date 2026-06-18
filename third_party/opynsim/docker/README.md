# `docker/` Linux-Based Development Environments

The release build uses `almalinux8.Dockerfile` to build/test/debug a
release. This produces binaries that have maximal compatibility with
a variety of Linux distributions. `ubuntu24.Dockerfile` is also provided
because it roughly reflects a typical desktop development environment.

## Cheatsheet

```bash
# List all images
docker images

# Delete `fooimage`
docker rmi fooimage

# List all containers
docker ps

# Prune all not-running containers
docker container prune

# Build a development environment image
cd docker && docker build -t opynsim:ubuntu24 -f ubuntu24.Dockerfile . && cd -

# Build a release environment image
cd docker && docker build -t opynsim:almalinux8 -f almalinux8.Dockerfile . && cd -

# Build project via development environment
#
# --init
#     Can be necessary to prevent the bash script from being PID=1, which can
#     make `xvfb-run` hang
#
# --device /dev/dri:/dev/dri
#     Can be necessary when running tests that use the renderer - even the software
#     renderer on some distros will probe `/dev/dri` to see what renderers could be
#     available.
#
# --volume
#     Volume maps here are opinionated: I prefer each build environment has its own
#     `build/` and `.venv` directory so that the build outputs can be ran from other
#     environments.
BUILD_ENV=almalinux8  # or ubuntu24
docker run                                               \
    --rm                                                 \
    --tty                                                \
    --interactive                                        \
    --init                                               \
    --device /dev/dri:/dev/dri                           \
    --volume ${PWD}:/project:ro                          \
    --volume ${PWD}/build-${BUILD_ENV}:/project/build:rw \
    --volume ${PWD}/.venv-${BUILD_ENV}:/project/.venv:rw \
    --workdir /project                                   \
    opynsim:${BUILD_ENV}
```

# Platform notes

- Running the test suite without any windowing system: `SDL_VIDEODRIVER=offscreen ctest --preset Development -j$(nproc)`
- Running the test suite in a fake Wayland instance: `export XDG_RUNTIME_DIR=$(mktemp -d) ; chmod 700 $XDG_RUNTIME_DIR && SDL_VIDEODRIVER=wayland WLR_BACKENDS=headless cage -- ctest --preset Development -j$(nproc)`
- Running the test suite with X11: `SDL_VIDEODRIVER=x11 xvfb-run ctest --preset Development -j$(nproc)`
