#!/usr/bin/env bash

# This script is presumed to run in the docker container
#
#     quay.io/pypa/manylinux_2_34_x86_64
#
# Example:
#     docker run --rm -v ${PWD}:/opynsim quay.io/pypa/manylinux_2_34_x86_64 bash -c "cd opynsim/ && ./scripts/e2e_setup-and-build-manylinux234.sh"
#
# manylinux_2_34 supports: Debian 12+, Ubuntu 21.10+, Fedora 35+, CentOS/RHEL 9+

# Install Ninja
yum install -y epel-release
yum install -y ninja-build

# Use Python 3.12
export PATH=/opt/python/cp312-cp312/bin:${PATH}

# Call into generic buildscript
./scripts/e2e_setup-and-build-unix.sh

