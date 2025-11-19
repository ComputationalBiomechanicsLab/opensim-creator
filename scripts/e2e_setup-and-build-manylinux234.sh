#!/usr/bin/env bash

# Performs an end-to-end build in a manylinux docker
# container, which should produce a build that's maximally
# compatible on many distributions of linux
#
# manylinux_2_34: Debian 12+, Ubuntu 21.10+, Fedora 35+, CentOS/RHEL 9+

sudo docker run --rm -v ${PWD}:/opynsim quay.io/pypa/manylinux_2_34_x86_64 bash -c 'yum install -y epel-release && yum install -y ninja-build && cd opynsim && PATH=/opt/python/cp312-cp312/bin:$PATH ./scripts/e2e_setup-and-build-unix.sh'
