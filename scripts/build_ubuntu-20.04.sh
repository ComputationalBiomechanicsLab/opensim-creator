#!/usr/bin/env bash
#
# `build_ubuntu-20.04.sh`: performs an end-to-end build of OpenSim Creator
# on Debian platforms
#
#     usage (must be ran in repository root): `bash build_ubuntu-20.04.sh`

# error out of this script if it fails for any reason
set -xeuo pipefail


# ----- handle external build parameters ----- #

# "base" build type to use when build types haven't been specified
OSC_BASE_BUILD_TYPE=${OSC_BASE_BUILD_TYPE:-Release}

# build type for all of OSC's dependencies
OSC_DEPS_BUILD_TYPE=${OSC_DEPS_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OSC
OSC_BUILD_TYPE=${OSC_BUILD_TYPE-`echo ${OSC_BASE_BUILD_TYPE}`}

# the argument passed to cmake's -G argument
OSC_BUILD_GENERATOR=${OSC_BUILD_GENERATOR-"Unix Makefiles"}

# maximum number of build jobs to run concurrently
#
# defaulted to 1, rather than `nproc`, because OpenSim requires a large
# amount of RAM--more than most machines have--to build concurrently, #659
OSC_BUILD_CONCURRENCY=${OSC_BUILD_CONCURRENCY:-1}

# extra flags to pass into each configuration call to cmake
#
# this can be useful for (e.g.) specifying -DCMAKE_OSX_SYSROOT
# and -G (generator) flags
OSC_CMAKE_CONFIG_EXTRA=${OSC_CMAKE_CONFIG_EXTRA:-""}

# which OSC build target to build
#
#     osc        just build the `osc` binary
#     package    package everything into a .deb installer
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

echo "----- printing system info -----"
df  # print disk usage
ls -la .  # print build dir contents
uname -a  # print distro details
${CC:-cc} --version
${CXX:-c++} --version
cmake --version
make --version

echo "----- building OSC's dependencies -----"
cmake \
    -G "${OSC_BUILD_GENERATOR}" \
    -S third_party \
    -B "osc-deps-build" \
    -DCMAKE_BUILD_TYPE=${OSC_DEPS_BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX="osc-deps-install"
    ${OSC_CMAKE_CONFIG_EXTRA}
cmake --build "osc-deps-build" -j${OSC_BUILD_CONCURRENCY}

echo "----- building OSC -----"
cmake \
    -G "${OSC_BUILD_GENERATOR}" \
    -S . \
    -B "osc-build" \
    -DCMAKE_BUILD_TYPE=${OSC_BUILD_TYPE} \
    -DCMAKE_PREFIX_PATH="${PWD}/osc-deps-install"
    ${OSC_BUILD_DOCS:+-DOSC_BUILD_DOCS=ON} \
    ${OSC_CMAKE_CONFIG_EXTRA}
cmake --build "osc-build" -j${OSC_BUILD_CONCURRENCY}

# ensure tests pass
ctest --test-dir osc-build -j ${OSC_BUILD_CONCURRENCY} --output-on-failure

# build final package
cmake --build "osc-build" --target ${OSC_BUILD_TARGET} -j${OSC_BUILD_CONCURRENCY}
