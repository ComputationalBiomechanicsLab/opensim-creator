#!/usr/bin/env bash

# Performs an end-to-end build in a manylinux docker
# container, which should produce a build that's maximally
# compatible on many distributions of linux
#
# manylinux_2_34: Debian 12+, Ubuntu 21.10+, Fedora 35+, CentOS/RHEL 9+

sudo docker run --rm -v ${PWD}:/opynsim quay.io/pypa/manylinux_2_34_x86_64 bash -c "$(cat <<'EOF'
set -xeuo pipefail

export PATH=/opt/python/cp312-cp312/bin:$PATH
export CMAKE_BUILD_PARALLEL_LEVEL=$(nproc)

# go into mapped directory
cd opynsim/

# setup virtual environment
./scripts/setup_venv.py

# build bundled dependencies
cd third_party/ && cmake --workflow --preset OPynSim_third-party_ManyLinux && cd -

# source the project-level virtual environment
source ./scripts/env_venv.sh

# build the project
cmake --workflow --preset OPynSim_ManyLinux
EOF
)"

