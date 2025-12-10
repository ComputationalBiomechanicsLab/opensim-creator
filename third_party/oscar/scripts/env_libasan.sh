ASAN_OPTIONS="\
 abort_on_error=1 \
 detect_leaks=1 \
 detect_stack_use_after_return=1 \
 detect_stack_use_after_scope=1 \
 check_initialization_order=1 \
 strict_init_order=1 \
 strict_string_checks=1 \
 alloc_dealloc_mismatch=1 \
 malloc_context_size=50 \
 free_detect_stack_use_after_return=1 \
 detect_invalid_pointer_pairs=2 \
 handle_segv=1 \
 handle_sigbus=1 \
 handle_abort=1 \
 handle_sigill=1 \
 handle_sigfpe=1 \
 symbolize=1 \
 detect_odr_violation=2 \
 halt_on_error=1 \
 verbosity=1 \
"
export ASAN_OPTIONS
