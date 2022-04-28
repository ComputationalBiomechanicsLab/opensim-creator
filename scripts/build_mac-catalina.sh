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
OSC_OPENSIM_REPO=${OSC_OPENSIM_REPO:-https://github.com/opensim-org/opensim-core}

# can be any branch/tag identifier from opensim
OSC_OPENSIM_REPO_BRANCH=${OSC_OPENSIM_REPO_BRANCH:-4.3}

# maximum number of build jobs to run concurrently
OSC_BUILD_CONCURRENCY=${OSC_BUILD_CONCURRENCY:-$(sysctl -n hw.physicalcpu)}

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
echo "    OSC_BUILD_CONCURRENCY = ${OSC_BUILD_CONCURRENCY}"
echo "    OSC_BUILD_TARGET = ${OSC_BUILD_TARGET}"
echo "    OSC_SKIP_BREW = ${OSC_SKIP_BREW:-OFF}"
echo "    OSC_SKIP_OPENSIM = ${OSC_SKIP_OPENSIM:-OFF}"
echo "    OSC_SKIP_OSC = ${OSC_SKIP_OSC:-OFF}"
echo "    OSC_BUILD_DOCS = ${OSC_BUILD_DOCS:-OFF}"
echo ""
set -x

if [[ -z ${OSC_SKIP_BREW:+x} ]]; then
    echo "----- getting system-level dependencies -----"

    # reinstall gcc, because OpenSim 4.2 depends on gfortran for
    # libBLAS. This is probably a misconfigured dependency in OS4.2,
    # because Mac actually contains libBLAS
    brew reinstall gcc

    # `wget`
    #
    # seems to be a transitive dependency of OpenSim 4.2 (Metis),
    # which uses wget to get other deps

    # `automake`
    #
    # seems to be a transitive dependency of OpenSim 4.2 (adolc)
    # uses `aclocal` for configuration
    brew install wget automake

    [[ ! -z ${OSC_BUILD_DOCS:+z} ]] && brew install python3
    [[ ! -z ${OSC_BUILD_DOCS:+z} ]] && pip3 install -r docs/requirements.txt

    echo "----- finished getting system-level dependencies -----"
else
    echo "----- skipping getting system-level dependencies (OSC_SKIP_BREW) is
    set -----"
fi

if [[ -z ${OSC_SKIP_OPENSIM:+x} ]]; then
    echo "----- downloading, building, and installing (locally) OpenSim -----"

    # get OpenSim 4.2 sources
    if [[ ! -d opensim-core/ ]]; then
        git clone \
            --single-branch \
            --branch "${OSC_OPENSIM_REPO_BRANCH}" \
            --depth=1 \
            "${OSC_OPENSIM_REPO}"
    fi

    echo "----- building OpenSim's dependencies -----"
    mkdir -p opensim-dependencies-build/
    cd opensim-dependencies-build/
    cmake ../opensim-core/dependencies \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=../opensim-dependencies-install \
        -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer"
    cmake --build . -- -j${OSC_BUILD_CONCURRENCY}
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
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer" \
        -DOPENSIM_WITH_CASADI=YES \
        -DOPENSIM_WITH_TROPTER=NO
    cmake --build . --target install -- -j${OSC_BUILD_CONCURRENCY}
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
        -DCMAKE_BUILD_TYPE=Release \
        ${OSC_BUILD_DOCS:+-DOSC_BUILD_DOCS=ON}
    cmake --build . --target ${OSC_BUILD_TARGET} -- -j${OSC_BUILD_CONCURRENCY}
    echo "DEBUG: listing contents of final build dir"
    ls .
    cd -
fi

