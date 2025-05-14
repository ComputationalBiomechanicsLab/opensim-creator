# Provides `OSC_STRICT_COMPILER_OPTIONS`, which targets in
# the project use `PRIVATE`ly, to avoid downstream projects
# from requiring these strict flags.

set(OSC_STRICT_COMPILER_OPTIONS

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>:

        # set the warning level very high
        /W4

        # treat all warnings as errors
        /WX

        # disable MSVC's permissive mode to ensure better ISO C++ conformance
        /permissive-

        # ensure `volatile` variables follow (less-strict) ISO standards
        /volatile:iso

        # ensure preprocessor is standards conformant
        /Zc:preprocessor

        # assume `new` throws when memory cannot be allocated (ISO conformance)
        /Zc:throwingNew

        # only handle standard, synchronous, C++ exceptions (ISO) and treat asynchronous
        # Windows/system structured exceptions as fatal, non-catchable, errors
        /EHsc
    >

    # gcc AND clang flags
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:

        # treat all warnings as errors
        -Werror

        # enable all basic warnings
        -Wall

        # enable extra warnings
        -Wextra

        # enable pedantically extra warnings
        -pedantic

        # warn if an uninitialized variable is initialized by itself
        -Winit-self

        # warn if data is casted to a higher alignment (e.g. char -> long)
        #
        # (set to =strict if the code quality is high enough and Clang in CI is high enough)
        -Wcast-align

        # warn if casting C string constants from `char const*` to `char*`
        -Wwrite-strings

        # warn if a dangling else is detected
        -Wdangling-else

        # warn if a date-time macro expansion is not reproducible
        -Wdate-time

        # warn if a variable-length array (VLA) is detected (disallowed)
        -Wvla

        # warn if the compiler detected that the code is way too complex to optimize
        -Wdisabled-optimization

        # warn if a structure is given the 'packed' attribute (disallowed: alignment)
        -Wpacked

        # warn if a case in a switch statement implicitly falls through after a statement
        -Wimplicit-fallthrough

        # warn if insecure string formatting (e.g. for printf) is detected
        -Wformat-security

        # disabled: requires newer gcc
        # warn if trying to allocate 0 bytes of memory using an allocation function (could be undef behavior)
        # -Walloc-zero

        # disabled: requires newer gcc
        # warn if using trampoline functions (requires executable stack)
        # -Wtrampolines

        # warn if a pointer is cast in a C-style cast in such a way that it removes qualifiers (e.g. char const* -> char*)
        -Wcast-qual

        # warn if implicit conversion may alter a value
        -Wconversion

        # disabled: very very hard to avoid all warnings from this :(
        #
        # warn if an implicit conversion involving signed-to-unsigned etc. may alter a value
        -Wno-sign-conversion

        # disabled: the codebase contains MSVC-specific `pragma warning` etc. that should hopefully
        #           be dropped once the codebase is upgraded to C++23
        -Wno-unknown-pragmas

        # disabled: requires newer gcc
        # warn if a suspicous use of a logical operator is detected (e.g. i < 0 && i < 0)
        # -Wlogical-op
    >

    # clang flags
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
        # warn if using an uninitialized variable
        #
        # not done on gcc because it produces false-positives
        -Wuninitialized
    >

    # gcc flags
    $<$<CXX_COMPILER_ID:GNU>:
        # produces false positives?
        -Wno-uninitialized

        # false-positive in googletest https://github.com/google/googletest/issues/4232
        -Wno-restrict

	# false-positives (e.g. https://github.com/fmtlib/fmt/issues/3415)
        -Wno-dangling-reference
    >

    $<$<CXX_COMPILER_ID:AppleClang>:
        # ensure AppleClang produces an error when targeting earlier versions of MacOS
        # with a newer SDK
        -Werror=unguarded-availability
        -Werror=unguarded-availability-new
    >
)
