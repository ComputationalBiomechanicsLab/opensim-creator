# `env_libasan.sh`: sets up an environment for running libASAN-enabled
# binaries with aggressive flags
#
#     usage (unix only): `source env_libasan.sh`

# - these are libASAN options that I use when debugging OSC
# - it assumes you have compiled OSC with libASAN, e.g.:
#     - CXXFLAGS="-fsanitize=address"
#     - LDFLAGS="-fsanitize=address"

# care: you might see "unknown module" in libASAN's trace
#
# it's because some libraries leak but are unloaded before ASAN
# has a chance to scan them:
#
# - https://github.com/google/sanitizers/issues/89
#
# the workaround is to compile a C library:
#     #include <stdio.h>
#     int dlclose(void *handle) {}
#
# with: `gcc -c in.c -fpic -o out.o && gcc -shared in.o -o out.so`
#
# then LD_PRELOAD it to overwrite unloading libraries: `LD_PRELOAD=${PWD}/out.so`

export ASAN_OPTIONS="strict_string_checks=true:malloc_context_size=30:check_initialization_order=false:detect_stack_use_after_return=true"
