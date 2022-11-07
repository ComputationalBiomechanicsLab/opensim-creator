#!/usr/bin/env bash

# Mac Catalina: end-2-end build
#
#     - this script should run to completion on a relatively clean
#       Catalina (OSX 10.5) machine with xcode, CMake, etc. installed


# error out of this script if it fails for any reason
set -xeuo pipefail


# ----- handle external build parameters ----- #

# where to clone the OpenSim source from
#
# handy to override if you are developing against a fork, locally, etc.
OSC_OPENSIM_REPO=${OSC_OPENSIM_REPO:-https://github.com/ComputationalBiomechanicsLab/opensim-core}

# can be any branch/tag identifier from opensim
OSC_OPENSIM_REPO_BRANCH=${OSC_OPENSIM_REPO_BRANCH:-opensim-creator}

# base build type: used if one of the below isn't overidden
OSC_BASE_BUILD_TYPE=${OSC_BASE_BUILD_TYPE:-Release}

# build type for OpenSim's dependencies (e.g. Simbody)
OSC_OPENSIM_DEPS_BUILD_TYPE=${OSC_OPENSIM_DEPS_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OpenSim
OSC_OPENSIM_BUILD_TYPE=${OSC_OPENSIM_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OSC
OSC_BUILD_TYPE=${OSC_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# extra compiler flags for the C++ compiler
OSC_CXX_FLAGS=${OSC_CXX_FLAGS:-`echo -fno-omit-frame-pointer`}

# maximum number of build jobs to run concurrently
OSC_BUILD_CONCURRENCY=${OSC_BUILD_CONCURRENCY:-$(sysctl -n hw.physicalcpu)}

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

# set this if you want to skip downloading + building OpenSim from
# source
#
# if you skip this, you may also need to set the CMAKE_PREFIX_PATH env var
#
#     OSC_SKIP_OPENSIM

# set this if you want to skip building OSC
#
#     OSC_SKIP_OSC

# set this if you want to build the documentation
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
echo "    OSC_SKIP_BREW = ${OSC_SKIP_BREW:-OFF}"
echo "    OSC_SKIP_OPENSIM = ${OSC_SKIP_OPENSIM:-OFF}"
echo "    OSC_SKIP_OSC = ${OSC_SKIP_OSC:-OFF}"
echo "    OSC_BUILD_DOCS = ${OSC_BUILD_DOCS:-OFF}"
echo "    OSC_GENERATOR = ${OSC_GENERATOR}"
echo "    OSC_CXX_FLAGS = ${OSC_CXX_FLAGS}"
echo ""
set -x


echo "----- ensuring all submodules are up-to-date -----"
git submodule update --init --recursive


if [[ -z ${OSC_SKIP_BREW:+x} ]]; then
    echo "----- getting system-level dependencies -----"

    # reinstall gcc
    #
    # this is necessary because OpenSim depends on gfortran for
    # libBLAS. This is probably a misconfigured dependency in OpenSim,
    # because Mac already contains an Apple-provided libBLAS
    brew reinstall gcc

    # `wget`
    #
    # seems to be a transitive dependency of OpenSim (Metis),
    # which uses wget to get other deps

    # `automake`
    #
    # seems to be a transitive dependency of OpenSim (adolc)
    # uses `aclocal` for configuration

    brew install wget

    # osc: docs dependencies
    [[ ! -z ${OSC_BUILD_DOCS:+z} ]] && brew install python3
    [[ ! -z ${OSC_BUILD_DOCS:+z} ]] && pip3 install -r docs/requirements.txt

    # ensure sphinx-build is available on this terminal's PATH
    [[ ! -z ${OSC_BUILD_DOCS:+z} ]] && export PATH=${PATH}:"$(python3 -m site --user-base)/bin"

    echo "----- finished getting system-level dependencies -----"
else
    echo "----- skipping getting system-level dependencies (OSC_SKIP_BREW) is
    set -----"
fi


echo "----- printing system (post-dependency install) info -----"
cc --version
c++ --version
cmake --version
make --version
[[ ! -z ${OSC_BUILD_DOCS:+z} ]] sphinx-build --version  # required when building docs


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

    # TODO: disable CASADI+Ipopt+metis once OpenSim 4.4 is released:
    #
    # https://github.com/opensim-org/opensim-core/pull/3206

    echo "----- building OpenSim's dependencies -----"
    mkdir -p opensim-dependencies-build/
    cd opensim-dependencies-build/
    cmake ../opensim-core/dependencies \
        -DCMAKE_BUILD_TYPE=${OSC_OPENSIM_DEPS_BUILD_TYPE} \
        -DCMAKE_INSTALL_PREFIX=../opensim-dependencies-install \
        -DCMAKE_CXX_FLAGS="${OSC_CXX_FLAGS}" \
        -DCMAKE_GENERATOR="${OSC_GENERATOR}" \
        -DOPENSIM_WITH_CASADI=OFF \
        -DOPENSIM_WITH_TROPTER=OFF
    cmake --build . --verbose -- -j${OSC_BUILD_CONCURRENCY}
    echo "DEBUG: listing contents of OpenSim dependencies build dir"
    ls .
    cd -

    echo "----- building OpenSim -----"
    mkdir -p opensim-build/
    cd opensim-build/
    cmake ../opensim-core/ \
        -DOPENSIM_DEPENDENCIES_DIR=../opensim-dependencies-install/ \
        -DCMAKE_INSTALL_PREFIX=../opensim-install/ \
        -DBUILD_JAVA_WRAPPING=OFF \
        -DCMAKE_BUILD_TYPE=${OSC_OPENSIM_BUILD_TYPE} \
        -DCMAKE_CXX_FLAGS=${OSC_CXX_FLAGS} \
        -DCMAKE_GENERATOR="${OSC_GENERATOR}" \
        -DOPENSIM_DISABLE_LOG_FILE=ON \
        -DOPENSIM_WITH_CASADI=OFF \
        -DOPENSIM_WITH_TROPTER=OFF \
        -DOPENSIM_COPY_DEPENDENCIES=ON
    cmake --build . --verbose --target install -- -j${OSC_BUILD_CONCURRENCY}
    echo "DEBUG: listing contents of OpenSim build dir"
    ls .
    cd -

    echo "----- finished building OpenSim -----"
else
    echo "----- skipping OpenSim build (OSC_SKIP_OPENSIM is set) -----"
fi

if [[ -z ${OSC_SKIP_OSC:+x} ]]; then
    echo "----- building OSC -----"

    mkdir -p osc-build/
    cd osc-build/
    cmake .. \
        -DCMAKE_PREFIX_PATH=${PWD}/../opensim-install \
        -DCMAKE_INSTALL_PREFIX=${PWD}/../osc-install \
        -DCMAKE_BUILD_TYPE=${OSC_BUILD_TYPE} \
        -DCMAKE_CXX_FLAGS=${OSC_CXX_FLAGS} \
        -DCMAKE_GENERATOR="${OSC_GENERATOR}" \
        ${OSC_BUILD_DOCS:+-DOSC_BUILD_DOCS=ON}
    cmake --build . --verbose --target ${OSC_BUILD_TARGET} -- -j${OSC_BUILD_CONCURRENCY}
    echo "DEBUG: listing contents of final build dir"
    ls .
    cd -
else
    echo "----- skipping OSC build (OSC_SKIP_OSC is set) -----"
fi
