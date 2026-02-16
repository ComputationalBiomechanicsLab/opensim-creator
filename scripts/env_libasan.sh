# Setup libASAN suitable for simbody/OpenSim/Creator
export ASAN_OPTIONS="abort_on_error=1:strict_string_checks=true:malloc_context_size=30:check_initialization_order=true:detect_stack_use_after_return=true:strict_init_order=true:detect_invalid_pointer_pairs=2"

# Add suppressions to libLSAN (usually, they're just OS suppressions)
export LSAN_OPTIONS="suppressions=${PWD}/scripts/suppressions_lsan.supp"

# Use software renderer (minimizes leaks/UB from vendor OpenGL driver).
export LIBGL_ALWAYS_SOFTWARE=1
