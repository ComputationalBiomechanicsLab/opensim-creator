#!/usr/bin/env bash
#
# Bundles the source code for a given git tag into a tarball archive.

if [[ "$#" -ne 1 ]]; then
    echo "usage: $0 GIT_TAG"
    exit 1
fi

set -xeuo pipefail

TAG="$1"
OUTPUT_PATH="opensimcreator-${TAG}-src.tar.xz"

git archive --format=tar --prefix=opensimcreator-${TAG}/ ${TAG} | xz -c > ${OUTPUT_PATH}
echo "source archive written to ${OUTPUT_PATH}"
