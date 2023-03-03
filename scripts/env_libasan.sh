# these are libASAN options that I use for strictly debugging OSC

# this assumes you have compiled OSC with libASAN (e.g. export CXXFLAGS=-fsanitize=address or similar)

export ASAN_OPTIONS="detect_container_overflow=0:fast_unwind_on_malloc=0:malloc_context_size=30:check_initialization_order=true:detect_stack_use_after_return=true:atexit=true:strict_init_order=true"