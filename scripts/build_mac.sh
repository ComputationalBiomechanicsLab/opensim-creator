#!/usr/bin/env bash

# `build_mac`: performs an end-to-end build of OpenSim Creator on the
# mac platform
#
#     usage (must be ran in repository root): `bash build_mac.sh`
#
# this assumes the system already has the necessary dependencies installed


# error out of this script if it fails for any reason
set -xeuo pipefail


# ----- handle external build parameters ----- #

# "base" build type to use when build types haven't been specified
OSC_BASE_BUILD_TYPE=${OSC_BASE_BUILD_TYPE:-Release}

# build type for all of OSC's dependencies
OSC_DEPS_BUILD_TYPE=${OSC_DEPS_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OSC
OSC_BUILD_TYPE=${OSC_BUILD_TYPE-`echo ${OSC_BASE_BUILD_TYPE}`}

# maximum number of build jobs to run concurrently
#
# defaulted to 1, rather than `sysctl -n hw.physicalcpu`, because OpenSim
# requires a large  amount of RAM--more than most machines have--to build
# concurrently, #659
OSC_BUILD_CONCURRENCY=${OSC_BUILD_CONCURRENCY:-1}

# extra flags to pass into each configuration call to cmake
#
# this can be useful for (e.g.) specifying -DCMAKE_OSX_SYSROOT
# and -G (generator) flags
OSC_CMAKE_CONFIG_EXTRA=${OSC_CMAKE_CONFIG_EXTRA:-""}

# which OSC build target to build
#
#     osc      just build the osc binary
#     package  package everything into a .dmg installer
OSC_BUILD_TARGET=${OSC_BUILD_TARGET:-package}

set +x
echo "----- starting build -----"
echo ""
echo "----- printing build parameters -----"
echo ""
echo "    OSC_BASE_BUILD_TYPE    = ${OSC_BASE_BUILD_TYPE}"
echo "    OSC_DEPS_BUILD_TYPE    = ${OSC_DEPS_BUILD_TYPE}"
echo "    OSC_BUILD_TYPE         = ${OSC_BUILD_TYPE}"
echo "    OSC_BUILD_CONCURRENCY  = ${OSC_BUILD_CONCURRENCY}"
echo "    OSC_CMAKE_CONFIG_EXTRA = ${OSC_CMAKE_CONFIG_EXTRA}"
echo "    OSC_BUILD_TARGET       = ${OSC_BUILD_TARGET}"
echo ""
set -x


echo "----- PATH -----"
echo "${PATH}"
echo "----- /PATH -----"

echo "----- printing system info -----"
cc --version
c++ --version
cmake --version
make --version
python3 --version

echo "----- building OSC's dependencies -----"
cmake \
    -S third_party \
    -B "osc-deps-build" \
    -DCMAKE_BUILD_TYPE=${OSC_DEPS_BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX="osc-deps-install" \
    ${OSC_CMAKE_CONFIG_EXTRA}

cmake \
    --build "osc-deps-build" \
    -j${OSC_BUILD_CONCURRENCY}

echo "----- building OSC -----"

# configure
cmake \
    -S . \
    -B "osc-build" \
    -DCMAKE_BUILD_TYPE=${OSC_BUILD_TYPE} \
    -DCMAKE_PREFIX_PATH="${PWD}/osc-deps-install" \
    ${OSC_CMAKE_CONFIG_EXTRA}

# build all
cmake \
    --build "osc-build" \
    -j${OSC_BUILD_CONCURRENCY}

# ensure tests pass
ctest --test-dir osc-build --output-on-failure -j${OSC_BUILD_CONCURRENCY}

# build final package

# FIXME: this is in a retry loop because packaging
# can sometimes fail in GitHub's macos-13 runner
for i in {1..8}; do
    set +e
    cmake \
        --build "osc-build" \
        --target ${OSC_BUILD_TARGET} \
        -j${OSC_BUILD_CONCURRENCY} && break ; sleep 2
    set -e
done
