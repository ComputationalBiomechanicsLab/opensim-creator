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
OSC_OPENSIM_REPO=https://github.com/opensim-org/opensim-core

# can be any branch identifier from opensim-core
OSC_OPENSIM_REPO_BRANCH=${OSC_OPENSIM_VERSION:-4.2}

# base build type: used if one of the below isn't overridden
OSC_BASE_BUILD_TYPE=${OSC_BASE_BUILD_TYPE:-RelWithDebInfo}

# build type for OpenSim's dependencies (e.g. Simbody)
OSC_OPENSIM_DEPS_BUILD_TYPE=${OSC_OPENSIM_DEPS_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OpenSim
OSC_OPENSIM_BUILD_TYPE=${OSC_OPENSIM_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OSC
OSC_BUILD_TYPE=${OSC_BUILD_TYPE-`echo ${OSC_BASE_BUILD_TYPE}`}

# maximum number of build jobs to run concurrently
OSC_BUILD_CONCURRENCY=${OSC_BUILD_CONCURRENCY:-$(nproc)}

# which build target to build
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


# ----- get system-level dependencies ----- #
if [[ ! -z ${OSC_SKIP_APT+x} ]]; then
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
fi


# ----- download, build, and install OpenSim ----- #
if [[ ! -z ${OSC_SKIP_OPENSIM+x} ]]; then

    # clone sources
    if [[ ! -d opensim-core/ ]]; then
        git clone \
            --single-branch \
            --branch "${OSC_OPENSIM_REPO_BRANCH}" \
            --depth=1 \
            "${OSC_OPENSIM_REPO}"
    fi

    # build OpenSim's dependencies
    mkdir -p opensim-dependencies-build/
    cd opensim-dependencies-build/
    cmake ../opensim-core/dependencies \
        -DCMAKE_BUILD_TYPE=${OSC_OPENSIM_DEPS_BUILD_TYPE} \
        -DCMAKE_INSTALL_PREFIX=../opensim-dependencies-install
    cmake --build . -j${OSC_BUILD_CONCURRENCY}
    echo "DEBUG: listing contents of OpenSim dependencies build dir"
    ls .
    cd -

    # build + install OpenSim
    mkdir -p opensim-build/
    cd opensim-build/
    cmake ../opensim-core/ \
        -DOPENSIM_DEPENDENCIES_DIR=../opensim-dependencies-install/ \
        -DCMAKE_INSTALL_PREFIX=../opensim-install/ \
        -DBUILD_JAVA_WRAPPING=OFF \
        -DCMAKE_BUILD_TYPE=${OSC_OPENSIM_BUILD_TYPE} \
        -DOPENSIM_WITH_CASADI=NO \
        -DOPENSIM_WITH_TROPTER=NO \
        -DOPENSIM_COPY_DEPENDENCIES=ON
    cmake --build . --target install -j${OSC_BUILD_CONCURRENCY}
    echo "DEBUG: listing contents of OpenSim build dir"
    ls .
    cd -
fi

# ----- build osc ----- #
if [[ ! -z ${OSC_SKIP_OSC+x} ]]; then
    mkdir -p osc-build/
    cd osc-build/
    cmake .. -DCMAKE_BUILD_TYPE=${OSC_BUILD_TYPE}
    cmake --build . --target ${OSC_BUILD_TARGET} -j${OSC_BUILD_CONCURRENCY}
    echo "DEBUG: listing contents of final build dir"
    ls .
    cd -
fi


# ----- install (example) -----

# apt-get install -yf ./osc-*.deb

# ----- boot (example) -----

# ./osc  # boot from the build dir
# osc    # boot the installed DEB (at /opt/osc/bin/osc)
