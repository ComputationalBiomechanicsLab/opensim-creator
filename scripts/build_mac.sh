#!/usr/bin/env bash

# `build_mac`: performs an end-to-end build of OpenSim Creator on the
# mac platform
#
#     usage (must be ran in repository root): `bash build_mac.sh`


# error out of this script if it fails for any reason
set -xeuo pipefail


# ----- handle external build parameters ----- #

# "base" build type to use when build types haven't been specified
OSC_BASE_BUILD_TYPE=${OSC_BASE_BUILD_TYPE:-Release}

# build type for all of OSC's dependencies
OSC_DEPS_BUILD_TYPE=${OSC_DEPS_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OSC
OSC_BUILD_TYPE=${OSC_BUILD_TYPE-`echo ${OSC_BASE_BUILD_TYPE}`}

# extra compiler flags for the C++ compiler
OSC_CXX_FLAGS=${OSC_CXX_FLAGS:-`echo -fno-omit-frame-pointer`}

# maximum number of build jobs to run concurrently
#
# defaulted to 1, rather than `sysctl -n hw.physicalcpu`, because OpenSim
# requires a large  amount of RAM--more than most machines have--to build
# concurrently, #659
OSC_BUILD_CONCURRENCY=${OSC_BUILD_CONCURRENCY:-1}

# which build system to use (e.g. Ninja, Makefile: see https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html)
OSC_GENERATOR=${OSC_GENERATOR:-`echo Unix Makefiles`}

# which OSC build target to build
#
#     osc      just build the osc binary
#     package  package everything into a .dmg installer
OSC_BUILD_TARGET=${OSC_BUILD_TARGET:-package}

# set this if you want to skip installing system-level deps
#
#     OSC_SKIP_BREW

# set this if you want to build the documentation
#
#     OSC_BUILD_DOCS

set +x
echo "----- starting build -----"
echo ""
echo "----- printing build parameters -----"
echo ""
echo "    OSC_BASE_BUILD_TYPE   = ${OSC_BASE_BUILD_TYPE}"
echo "    OSC_DEPS_BUILD_TYPE   = ${OSC_DEPS_BUILD_TYPE}"
echo "    OSC_BUILD_TYPE        = ${OSC_BUILD_TYPE}"
echo "    OSC_CXX_FLAGS         = ${OSC_CXX_FLAGS}"
echo "    OSC_BUILD_CONCURRENCY = ${OSC_BUILD_CONCURRENCY}"
echo "    OSC_GENERATOR         = ${OSC_GENERATOR}"
echo "    OSC_BUILD_TARGET      = ${OSC_BUILD_TARGET}"
echo "    OSC_SKIP_BREW         = ${OSC_SKIP_BREW:-OFF}"
echo "    OSC_BUILD_DOCS        = ${OSC_BUILD_DOCS:-OFF}"
echo ""
set -x


echo "----- ensuring all submodules are up-to-date -----"
git submodule update --init --recursive


if [[ -z ${OSC_SKIP_BREW:+x} ]]; then
    echo "----- getting system-level dependencies -----"

    # `wget`
    #
    # seems to be a transitive dependency of OpenSim (Metis),
    # which uses wget to get other deps
    # brew install wget  # unnecessary: it's usually already installed

    # osc: docs dependencies
    # [[ ! -z ${OSC_BUILD_DOCS:+z} ]] && brew install python3  # unnecessary: it's usually already installed
    [[ ! -z ${OSC_BUILD_DOCS:+z} ]] && pip3 install --user wheel
    [[ ! -z ${OSC_BUILD_DOCS:+z} ]] && pip3 install --user -r docs/requirements.txt

    # ensure sphinx-build is available on this terminal's PATH
    [[ ! -z ${OSC_BUILD_DOCS:+z} ]] && ls "$(python3 -m site --user-base)/bin"
    [[ ! -z ${OSC_BUILD_DOCS:+z} ]] && export PATH=${PATH}:"$(python3 -m site --user-base)/bin"

    echo "----- finished getting system-level dependencies -----"
else
    echo "----- skipping getting system-level dependencies (OSC_SKIP_BREW) is
    set -----"
fi

echo "----- PATH -----"
echo "${PATH}"
echo "----- /PATH -----"

echo "----- printing system (post-dependency install) info -----"
cc --version
c++ --version
cmake --version
make --version
[[ ! -z ${OSC_BUILD_DOCS:+z} ]] && sphinx-build --version  # required when building docs

echo "----- building OSC's dependencies -----"
cmake \
    -S third_party \
    -B "osc-deps-build" \
    -DCMAKE_BUILD_TYPE=${OSC_DEPS_BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX="osc-deps-install"
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
    ${OSC_BUILD_DOCS:+-DOSC_BUILD_DOCS=ON}

# build all
cmake \
    --build "osc-build" \
    -j${OSC_BUILD_CONCURRENCY}

# ensure tests pass
ctest --test-dir osc-build --output-on-failure

# build final package
cmake \
    --build "osc-build" \
    --target ${OSC_BUILD_TARGET} \
    -j${OSC_BUILD_CONCURRENCY}
