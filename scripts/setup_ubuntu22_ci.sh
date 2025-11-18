#!/usr/bin/env bash

set -xeuo pipefail

sudo apt-get install \
    $(cat scripts/ubuntuXX_apt_build-dependencies.txt) \
    $(cat scripts/ubuntu22_apt_build-dependencies.txt) \
    $(cat scripts/ubuntu22_apt_ci-dependencies.txt)
