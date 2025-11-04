#!/usr/bin/env bash
#
# Builds the project's third-party dependencies on Linux, ready for the
# main build to proceed (either via an IDE or build script).

exec ./scripts/setup_deps_unix.sh "$@"
