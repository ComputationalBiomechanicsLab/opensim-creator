function(opyn_add_sanitizer_flags_to target)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all)
    target_link_options(${target} PUBLIC -fsanitize=address,undefined -rdynamic)
endfunction()
