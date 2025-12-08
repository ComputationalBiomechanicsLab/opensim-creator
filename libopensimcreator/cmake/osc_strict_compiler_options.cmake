# A set of compiler flags that can be used in `target_compile_options` to
# compile a target with stricter rules than the compiler's default.
set(OSC_STRICT_COMPILER_OPTIONS

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>:
        /WX               # treat all warnings as errors
        /W4               # set the warning level very high
        /permissive-      # disable MSVC's permissive mode to ensure better ISO C++ conformance
        /volatile:iso     # ensure `volatile` variables follow (less-strict) ISO standards
        /Zc:preprocessor  # ensure preprocessor is standards conformant
        /Zc:throwingNew   # assume `new` throws when memory cannot be allocated (ISO conformance)
        /EHsc             # only handle standard, synchronous, C++ exceptions (ISO), asynchronous structured exceptions are fatal and non-catchable
    >

    # gcc/clang flags
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
        -Werror                  # treat all warnings as errors
        -Wall                    # enable all basic warnings
        -Wextra                  # enable extra warnings
        -pedantic                # enable pedantically extra warnings
        -Winit-self              # warn if an uninitialized variable is initialized by itself
        -Wcast-align             # warn if data is casted to a higher alignment (e.g. char -> long)
        -Wwrite-strings          # warn if casting C string constants from `char const*` to `char*`
        -Wdangling-else          # warn if a dangling else is detected
        -Wdate-time              # warn if a date-time macro expansion is not reproducible
        -Wvla                    # warn if a variable-length array (VLA) is detected (disallowed)
        -Wdisabled-optimization  # warn if the compiler detected that the code is way too complex to optimize
        -Wpacked                 # warn if a structure is given the 'packed' attribute (disallowed: alignment)
        -Wimplicit-fallthrough   # warn if a case in a switch statement implicitly falls through after a statement
        -Wformat-security        # warn if insecure string formatting (e.g. for printf) is detected
        -Wcast-qual              # warn if a pointer is cast in a C-style cast in such a way that it removes qualifiers (e.g. char const* -> char*)
        -Wconversion             # warn if implicit conversion may alter a value

        -Wno-sign-conversion     # don't warn if an implicit conversion involving signed-to-unsigned etc. may alter a value (hard to implement)
        -Wno-unknown-pragmas     # don't warn if the codebase contains unknown `pragma`s (e.g. MSVC-specific `pragma warning` etc.)
        -Wno-extra-semi          # don't warn if extra semicolons are detected: some macro expansions can write additional semicolons
    >

    # clang flags
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
        -Wuninitialized  # warn if using an uninitialized variable
    >

    # gcc flags
    $<$<CXX_COMPILER_ID:GNU>:
        -Wtrampolines            # warn if using trampoline functions (requires executable stack)
        -Wlogical-op             # warn if a suspicous use of a logical operator is detected (e.g. i < 0 && i < 0)

        -Wno-uninitialized       # don't warn if using an uninitialized variable (produces false positives in some versions of gcc)
        -Wno-restrict            # don't warn if `restrict` is misused (produces false positives in googletest: https://github.com/google/googletest/issues/4232)
        -Wno-dangling-reference  # don't warn if a dangling reference is detected (produces false positives, e.g. https://github.com/fmtlib/fmt/issues/3415)
    >

    $<$<CXX_COMPILER_ID:AppleClang>:
        -Werror=unguarded-availability      # error if targeting earlier versions of MacOS with a newer SDK (can produce invalid binaries?)
        -Werror=unguarded-availability-new  # error if targeting earlier versions of MacOS with a newer SDK (can produce invalid binaries?)
    >
)
