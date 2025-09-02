#!/usr/bin/env bash

# Performs an end-to-end build of OpenSim Creator with debugging options
# enabled. This is usaully used to verify a release, or lint the source
# code.

set -xeuo pipefail

OSC_BUILD_CONCURRENCY=${OSC_BUILD_CONCURRENCY:-$(nproc)}
export CC=${CC:-clang}
export CXX=${CXX:-clang++}
export CLANG_TIDY=${CLANG_TIDY:-clang-tidy}

mkdir -p build/

# Create stub that hides graphics driver memory leaks (out of our control)
cat << EOF > build/dlclose.c
#include <stdio.h>
int dlclose(void *handle) {
    return 0;
}
EOF
${CC} -shared -o build/libdlclose.so -fPIC build/dlclose.c

# Create suppressions file for known leaks
cat << EOF > build/libasan_suppressions.supp
leak:/lib/x86_64-linux-gnu/libnvidia-glcore.so
leak:OpenSim::Coordinate
EOF

# configure+build dependencies
CCFLAGS=-fsanitize=address CXXFLAGS=-fsanitize=address cmake \
    -S third_party/ \
    -B third_party-build \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX="${PWD}/third_party-install"
cmake --build third_party-build/ --verbose -j${OSC_BUILD_CONCURRENCY}

# configure+build OpenSimCreator
# also: `-DCMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use;-Xiwyu;any;-Xiwyu;iwyu;-Xiwyu;"`
# temporarily disabled because simbody/OpenSim suck: `CCFLAGS="-fsanitize=address,undefined,bounds -fno-sanitize-recover=all" CXXFLAGS="-fsanitize=address,undefined,bounds -fno-sanitize-recover=all"

CCFLAGS="-fsanitize=address -fno-sanitize-recover=all" CXXFLAGS="-fsanitize=address -fno-sanitize-recover=all" LDFLAGS="-rdynamic" cmake \
    -S . \
    -B build/ \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DOSC_FORCE_ASSERTS_ENABLED=ON \
    -DCMAKE_PREFIX_PATH="${PWD}/third_party-install" \
    -DCMAKE_CXX_CLANG_TIDY=${CLANG_TIDY}
cmake --build build/ --verbose -j${OSC_BUILD_CONCURRENCY}

# run tests
# export UBSAN_OPTIONS="print_stacktrace=1"  # requires llvm symbolizer in PATH https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
export ASAN_OPTIONS="abort_on_error=1:strict_string_checks=true:malloc_context_size=30:check_initialization_order=true:detect_stack_use_after_return=true:strict_init_order=true"
export LIBGL_ALWAYS_SOFTWARE=1  # minimize driver leaks
export LD_PRELOAD=build/libdlclose.so  # minimize library unloading leaks (issue in graphics drivers, sometimes)
export LSAN_OPTIONS="suppressions=build/libasan_suppressions.supp"
./build/liboscar/testing/testoscar
./build/liboscar-demos/testing/testoscar_demos
ASAN_OPTIONS="${ASAN_OPTIONS}:check_initialization_order=false:strict_init_order=false" ./build/libopensimcreator/testing/TestOpenSimCreator
