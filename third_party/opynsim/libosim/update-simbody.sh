#!/usr/bin/env bash

set -xeuo pipefail

# this is just an example, substitute where necessary
subproject=simbody
subproject_url=https://github.com/simbody/simbody
subproject_branch=master

rm -rf "${subproject}"
git clone --depth=1 --branch="${subproject_branch}" "${subproject_url}" "${subproject}"
git --git-dir "${subproject}/.git" rev-parse HEAD > "${subproject}.version"
rm -rf "${subproject}/.git"

# apply any downstream patches
cd "${subproject}"
for patch in $(find "../${subproject}-patches" -name "*.patch"); do
    patch -p1 < "${patch}"
done
cd -
