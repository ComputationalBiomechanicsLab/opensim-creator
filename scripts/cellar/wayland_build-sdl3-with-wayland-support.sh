#!/usr/bin/env bash

# runtime: `apt install libdecor-0-0`, so that windows have decorations
# build:   `apt install libdecor-0-dev libwayland-dev` (see above)

cmake -S third_party/SDL -B sdl-build \
    -DCMAKE_INSTALL_PREFIX=${PWD}/osc-deps-install \
    -DSDL_INSTALL_CMAKEDIR=osc-deps-install/cmake/SDL3 \
    -DSDL_SHARED:BOOL=OFF \
    -DSDL_STATIC:BOOL=ON \
    -DSDL_TEST_LIBRARY:BOOL=OFF \
    -DSDL_PRESEED:BOOL=OFF \
    -DSDL_WAYLAND:BOOL=ON \
    -DSDL_X11:BOOL=ON \
    -DSDL_WAYLAND_LIBDECOR=ON

cmake --build sdl-build --target install -j$(nproc)

