#!/usr/bin/env bash

set -xeuo pipefail

git ls-files --recurse-submodules | tar caf ../opensimcreator-${1}-src.tar.xz --xform s:^:opensimcreator-${1}/: --verbatim-files-from -T-

