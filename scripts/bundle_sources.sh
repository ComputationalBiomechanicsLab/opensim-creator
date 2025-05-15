#!/usr/bin/env bash
#
# Bundles OpenSim Creator's sources into a single tarball, suitable for
# archiving.

set -xeuo pipefail

git ls-files --recurse-submodules | tar caf ../opensimcreator-${1}-src.tar.xz --xform s:^:opensimcreator-${1}/: --verbatim-files-from -T-
