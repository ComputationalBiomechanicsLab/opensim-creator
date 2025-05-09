#!/usr/bin/env bash
#
# Performs an end-to-end build of OpenSim Creator with build flags
# that make it valgrind-compatible. This is handy for debugging things
# like memory accesses, segfaults, etc.

set -xeuo pipefail

cmake \
    -S third_party/ \
    -B third_party-build \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX="${PWD}/third_party-install"
cmake --build third_party-build -j$(nproc)
cmake \
    -S . \
    -B build/ \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH="${PWD}/third_party-install"
cmake --build build/ -j$(nproc)

export LIBGL_ALWAYS_SOFTWARE=1
tmp=$(mktemp /tmp/valgrind_suppressions.XXXX.supp)
cat << 'EOF' > $tmp
{
    Memcheck:Leak
    obj:/usr/lib/wsl/lib/*.so
    obj:/usr/lib/x86_64-linux-gnu/libgobject-2.0.*
}
EOF
valgrind_cmd="valgrind --leak-check=full --trace-children=yes --suppressions=${tmp}"
${valgrind_cmd} ctest --test-dir build/ --output-on-failure
