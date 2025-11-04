#!/usr/bin/env bash
#
# Performs an end-to-end build of OpenSim Creator MacOS.


# error out of this script if it fails for any reason
set -xeuo pipefail


# ----- handle external build parameters ----- #

# "base" build type to use when build types haven't been specified
OSC_BASE_BUILD_TYPE=${OSC_BASE_BUILD_TYPE:-Release}

# build type for all of OSC's dependencies
OSC_DEPS_BUILD_TYPE=${OSC_DEPS_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OSC
OSC_BUILD_TYPE=${OSC_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# maximum number of build jobs to run concurrently
OSC_BUILD_CONCURRENCY=${OSC_BUILD_CONCURRENCY:-$(sysctl -n hw.ncpu)}

# extra flags to pass into each configuration call to cmake
#
# this can be useful for (e.g.) specifying -DCMAKE_OSX_SYSROOT
# and -G (generator) flags
OSC_CMAKE_CONFIG_EXTRA=${OSC_CMAKE_CONFIG_EXTRA:-""}

# which OSC build target to build
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
    -B third_party-build-${OSC_DEPS_BUILD_TYPE} \
    -DCMAKE_BUILD_TYPE=${OSC_DEPS_BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX=third_party-install-${OSC_DEPS_BUILD_TYPE} \
    ${OSC_CMAKE_CONFIG_EXTRA}
cmake --build third_party-build-${OSC_DEPS_BUILD_TYPE} --verbose -j${OSC_BUILD_CONCURRENCY}

echo "----- building OSC -----"
cmake \
    -S . \
    -B "build/" \
    -DCMAKE_BUILD_TYPE=${OSC_BUILD_TYPE} \
    -DCMAKE_PREFIX_PATH="${PWD}/third_party-install-${OSC_DEPS_BUILD_TYPE}" \
    ${OSC_CMAKE_CONFIG_EXTRA}
cmake --build "build/" --verbose -j${OSC_BUILD_CONCURRENCY}

# ensure tests pass
ctest --test-dir build/ --output-on-failure -j${OSC_BUILD_CONCURRENCY}

# build final package
#
# this is in a retry loop because some CI servers have
# wonky packaging behavior.
for i in {1..8}; do
    set +e
    cmake \
        --build "build/" \
        --target ${OSC_BUILD_TARGET} \
        --verbose \
        -j${OSC_BUILD_CONCURRENCY} && break ; sleep 2
    set -e
done
