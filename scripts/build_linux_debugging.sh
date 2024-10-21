#!/usr/bin/env bash
#
# `build_linux_debugging.sh`: performs an end-to-end build of OpenSim Creator with
# as many debugging options enabled as possible. This is usaully used to verify a
# release, or lint the source code
#
#     usage (must be ran in repository root): `bash build_linux_debugging.sh`

set -xeuo pipefail
OSC_BUILD_CONCURRENCY=$(nproc)

mkdir -p osc-build

# create stub that hides graphics driver memory leaks (out of my control)
cat << EOF > osc-build/dlclose.c
#include <stdio.h>
int dlclose(void *handle) {
    ;
}
EOF
gcc -shared -o osc-build/libdlclose.so -fPIC osc-build/dlclose.c

# create suppressions file for OpenSim, which contains this leak:
# #11 0x7f99fcf96cbc in OpenSim::FreeJoint::FreeJoint(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, OpenSim::PhysicalFrame const&, OpenSim::PhysicalFrame const&) /home/adam/opensim-creator/third_party/opensim-core/OpenSim/Simulation/SimbodyEngine/FreeJoint.h:96:25
# #12 0x7f99fbf4a099 in OpenSim::Component::finalizeConnections(OpenSim::Component&) /home/adam/opensim-creator/third_party/opensim-core/OpenSim/Common/Component.cpp:332:5
cat << EOF > osc-build/opensim_suppressions.supp
leak:OpenSim::Coordinate
EOF

export CC=clang
export CXX=clang++

# configure+build dependencies
CCFLAGS=-fsanitize=address CXXFLAGS=-fsanitize=address cmake -S third_party/ -B osc-deps-build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=${PWD}/osc-deps-install
cmake --build osc-deps-build/ -v -j${OSC_BUILD_CONCURRENCY}

# configure+build OpenSimCreator
# also: `-DCMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use;-Xiwyu;any;-Xiwyu;iwyu;-Xiwyu;"`

CCFLAGS="-fsanitize=address,undefined,bounds -fno-sanitize-recover=all" CXXFLAGS="-fsanitize=address,undefined,bounds -fno-sanitize-recover=all" cmake -S . -B osc-build -DCMAKE_BUILD_TYPE=Debug -DOSC_FORCE_ASSERTS_ENABLED=ON -DCMAKE_PREFIX_PATH=${PWD}/osc-deps-install -DCMAKE_INSTALL_PREFIX=${PWD}/osc-install -DCMAKE_CXX_CLANG_TIDY=clang-tidy -DCMAKE_EXECUTABLE_ENABLE_EXPORTS=ON
cmake --build osc-build -j${OSC_BUILD_CONCURRENCY}
cmake --build osc-build -j${OSC_BUILD_CONCURRENCY} --target testoscar
cmake --build osc-build -j${OSC_BUILD_CONCURRENCY} --target testoscar_demos
cmake --build osc-build -j${OSC_BUILD_CONCURRENCY} --target TestOpenSimCreator

# run tests
# export UBSAN_OPTIONS="print_stacktrace=1"  # requires llvm symbolizer in PATH https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
export ASAN_OPTIONS="abort_on_error=1:strict_string_checks=true:malloc_context_size=30:check_initialization_order=true:detect_stack_use_after_return=true:strict_init_order=true"
export LIBGL_ALWAYS_SOFTWARE=1  # minimize driver leaks
export LD_PRELOAD=osc-build/libdlclose.so  # minimize library unloading leaks (due to poor library design)
./osc-build/tests/testoscar/testoscar
./osc-build/tests/testoscar_demos/testoscar_demos
LSAN_OPTIONS="suppressions=osc-build/opensim_suppressions.supp" ASAN_OPTIONS="${ASAN_OPTIONS}:check_initialization_order=false:strict_init_order=false" ./osc-build/tests/TestOpenSimCreator/TestOpenSimCreator
