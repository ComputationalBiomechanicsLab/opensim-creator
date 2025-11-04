#!/usr/bin/env bash
#
# Builds the project's third-party dependencies on MacOS, ready for the
# main build to proceed (either via an IDE or build script).

set -xeuo pipefail

exec ./scripts/setup_deps_unix.sh "$@"

