#!/usr/bin/env bash

# Ubuntu 20 (Groovy) / Debian Buster: end-2-end build
#
#     - this script should run to completion on a clean install of the
#       OSes and produce a ready-to-use osc build
#
#     - run this from the repo root (opensim-creator) dir


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
# defaulted to 1, rather than `nproc`, because OpenSim requires a large
# amount of RAM--more than most machines have--to build concurrently, #659
OSC_BUILD_CONCURRENCY=${OSC_BUILD_CONCURRENCY:-1}

# which OSC build target to build
#
#     osc        just build the `osc` binary
#     package    package everything into a .deb installer
OSC_BUILD_TARGET=${OSC_BUILD_TARGET:-package}

# set this if you want to skip installing system-level deps
#
#     OSC_SKIP_APT

# set this if you want to build the docs
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
echo "    OSC_BUILD_CONCURRENCY = ${OSC_BUILD_CONCURRENCY}"
echo "    OSC_BUILD_TARGET      = ${OSC_BUILD_TARGET}"
echo "    OSC_SKIP_APT          = ${OSC_SKIP_APT:-OFF}"
echo "    OSC_BUILD_DOCS        = ${OSC_BUILD_DOCS:-OFF}"
echo ""
set -x


echo "----- printing system (pre-dependency install) info -----"
df  # print disk usage
ls -la .  # print build dir contents
uname -a  # print distro details


echo "----- ensuring all submodules are up-to-date -----"
git submodule update --init --recursive


if [[ -z ${OSC_SKIP_APT:+x} ]]; then
    echo "----- getting system-level dependencies -----"

    # if root is running this script then do not use `sudo` (some distros
    # do not have 'sudo' available)
    if [[ "${UID}" == 0 ]]; then
        sudo=''
    else
        sudo='sudo'
    fi

    ${sudo} apt-get update

    # osc: transitive dependencies from OpenSim
    ${sudo} apt-get install -y git freeglut3-dev libxi-dev libxmu-dev liblapack-dev wget

    # osc: main dependencies
    ${sudo} apt-get install -y build-essential cmake libsdl2-dev libgtk-3-dev

    # osc: docs dependencies (if OSC_BUILD_DOCS is set)
    [[ ! -z "${OSC_BUILD_DOCS:+z}" ]] && ${sudo} apt-get install python3 python3-pip
    [[ ! -z "${OSC_BUILD_DOCS:+z}" ]] && ${sudo} pip3 install -r docs/requirements.txt

    echo "----- finished getting system-level dependencies -----"
else
    echo "----- skipping getting system-level dependencies (OSC_SKIP_APT is set) -----"
fi


echo "----- printing system (post-dependency install) info -----"
cc --version
c++ --version
cmake --version
make --version

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
cmake .. \
    -S . \
    -B "osc-build" \
    -DCMAKE_BUILD_TYPE=${OSC_BUILD_TYPE} \
    -DCMAKE_PREFIX_PATH="${PWD}/osc-deps-install" \
    ${OSC_BUILD_DOCS:+-DOSC_BUILD_DOCS=ON}

# build tests and the final package
cmake \
    --build "osc-build" \
    --target testosc ${OSC_BUILD_TARGET} \
    -j${OSC_BUILD_CONCURRENCY}

# ensure tests pass
osc-build/testosc
