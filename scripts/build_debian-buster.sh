#!/usr/bin/env bash

# Ubuntu 20 (Groovy) / Debian Buster: end-2-end build
#
#     - this script should run to completion on a clean install of the
#       OSes and produce a ready-to-use osc build
#
#     - run this from the repo root dir


# error out of this script if it fails for any reason
set -xeuo pipefail


# ----- handle external build parameters ----- #

# where to clone the OpenSim source from
#
# handy to override if you are developing against a fork, locally, etc.
OSC_OPENSIM_REPO=${OSC_OPENSIM_REPO:-https://github.com/opensim-org/opensim-core}

# can be any branch identifier from opensim-core
OSC_OPENSIM_REPO_BRANCH=${OSC_OPENSIM_REPO_BRANCH:-4.4}

# base build type: used if one of the below isn't overridden
OSC_BASE_BUILD_TYPE=${OSC_BASE_BUILD_TYPE:-Release}

# build type for OpenSim's dependencies (e.g. Simbody)
OSC_OPENSIM_DEPS_BUILD_TYPE=${OSC_OPENSIM_DEPS_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OpenSim
OSC_OPENSIM_BUILD_TYPE=${OSC_OPENSIM_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OSC
OSC_BUILD_TYPE=${OSC_BUILD_TYPE-`echo ${OSC_BASE_BUILD_TYPE}`}

# maximum number of build jobs to run concurrently
OSC_BUILD_CONCURRENCY=${OSC_BUILD_CONCURRENCY:-$(nproc)}

# which OSC build target to build
#
#     osc        just build the osc binary
#     package    package everything into a .deb installer
OSC_BUILD_TARGET=${OSC_BUILD_TARGET:-package}

# set this if you want to skip installing system-level deps
#
#     OSC_SKIP_APT

# set this if you want to skip downloading + building OpenSim
# from source
#
# if you skip this, you may also need to set the `CMAKE_PREFIX_PATH`
# environment variable to point to an existing OpenSim install. The
# GetDependencies.cmake file in OSC will try and locate OpenSim
# automatically but you might need to give it a hint
#
#     OSC_SKIP_OPENSIM

# set this if you want to skip building OSC
#
#     OSC_SKIP_OSC

# set this if you want to build the docs
#
#     OSC_BUILD_DOCS

set +x
echo "----- starting build -----"
echo ""
echo "----- printing build parameters -----"
echo ""
echo "    OSC_OPENSIM_REPO = ${OSC_OPENSIM_REPO}"
echo "    OSC_OPENSIM_REPO_BRANCH = ${OSC_OPENSIM_REPO_BRANCH}"
echo "    OSC_BASE_BUILD_TYPE = ${OSC_BASE_BUILD_TYPE}"
echo "    OSC_OPENSIM_DEPS_BUILD_TYPE = ${OSC_OPENSIM_DEPS_BUILD_TYPE}"
echo "    OSC_OPENSIM_BUILD_TYPE = ${OSC_OPENSIM_BUILD_TYPE}"
echo "    OSC_BUILD_TYPE = ${OSC_BUILD_TYPE}"
echo "    OSC_BUILD_CONCURRENCY = ${OSC_BUILD_CONCURRENCY}"
echo "    OSC_BUILD_TARGET = ${OSC_BUILD_TARGET}"
echo ""
set -x


echo "----- printing system (pre-dependency install) info -----"
df  # print disk usage
ls -la .  # print build dir contents
uname -a  # print distro details


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


if [[ -z ${OSC_SKIP_OPENSIM:+x} ]]; then
    echo "----- downloading, building, and installing (locally) OpenSim -----"

    # clone sources
    if [[ ! -d opensim-core/ ]]; then
        git clone \
            --single-branch \
            --branch "${OSC_OPENSIM_REPO_BRANCH}" \
            --depth=1 \
            "${OSC_OPENSIM_REPO}"
    fi

    echo "--- building OpenSim's dependencies ---"
    mkdir -p opensim-dependencies-build/
    cd opensim-dependencies-build/
    cmake ../opensim-core/dependencies \
        -DCMAKE_BUILD_TYPE=${OSC_OPENSIM_DEPS_BUILD_TYPE} \
        -DCMAKE_INSTALL_PREFIX=../opensim-dependencies-install \
        -DOPENSIM_WITH_CASADI=OFF \
        -DOPENSIM_WITH_TROPTER=OFF
    cmake --build . -j${OSC_BUILD_CONCURRENCY}
    echo "DEBUG: listing contents of OpenSim dependencies build dir"
    ls .
    cd -

    echo "--- building OpenSim ---"
    mkdir -p opensim-build/
    cd opensim-build/
    cmake ../opensim-core/ \
        -DOPENSIM_DEPENDENCIES_DIR=../opensim-dependencies-install/ \
        -DCMAKE_INSTALL_PREFIX=../opensim-install/ \
        -DBUILD_JAVA_WRAPPING=OFF \
        -DCMAKE_BUILD_TYPE=${OSC_OPENSIM_BUILD_TYPE} \
        -DOPENSIM_DISABLE_LOG_FILE=ON \
        -DOPENSIM_WITH_CASADI=OFF \
        -DOPENSIM_WITH_TROPTER=OFF \
        -DOPENSIM_COPY_DEPENDENCIES=ON
    cmake --build . --target install -j${OSC_BUILD_CONCURRENCY}
    echo "DEBUG: listing contents of OpenSim build dir"
    ls .
    cd -

    echo "----- finished downloading, building, and installing OpenSim -----"
else
    echo "----- skipping OpenSim build (OSC_SKIP_OPENSIM is set) -----"
fi

if [[ -z ${OSC_SKIP_OSC:+x} ]]; then
    echo "----- building OSC -----"

    mkdir -p osc-build/
    cd osc-build/
    cmake .. \
        -DCMAKE_BUILD_TYPE=${OSC_BUILD_TYPE} \
        -DCMAKE_PREFIX_PATH=${PWD}/../opensim-install \
        -DCMAKE_INSTALL_PREFIX=${PWD}/../osc-install \
        ${OSC_BUILD_DOCS:+-DOSC_BUILD_DOCS=ON}
    cmake --build . --target ${OSC_BUILD_TARGET} -j${OSC_BUILD_CONCURRENCY}
    echo "DEBUG: listing contents of final build dir"
    ls .
    cd -

    echo "----- finished building OSC (yay!) -----"
    echo ""
    echo "  depending on what target you built, you should be able to run commands like:"
    echo ""
    echo "      osc-build/osc  # run the osc binary"
    echo "      apt-get install -yf osc-build/osc-*.deb  # install the package, then run 'osc'"
else
    echo "----- skipping OSC build (OSC_SKIP_OSC is set) -----"
fi
