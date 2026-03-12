#!/usr/bin/env bash
#
# Sets up the local filesystem to inline oscar, which is
# a related project that can be edited in-place.

set -xeuo pipefail

rm -rf third_party/oscar
git clone https://github.com/adamkewley/oscar third_party/oscar
