#!/usr/bin/env bash

# ci_bundle_sources.sh
#
# Bundle the source code for a given git tag into an archive


if [[ "$#" -ne 1 ]]; then
    echo "usage: $0 GIT_TAG"
    exit 1
fi

set -xeuo pipefail

TAG="$1"
OUTPUT_PATH="opensimcreator-${TAG}-src.tar.xz"

git archive --format=tar.xz --prefix=opensimcreator-${TAG}/ ${TAG} | xz -c > ${OUTPUT_PATH}
echo "source archive written to ${OUTPUT_PATH}"

