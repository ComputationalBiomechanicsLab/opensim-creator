# -------------- gather user-facing build cache vars ---------------- #

set(
    OSC_FORCE_ASSERTS_ENABLED ON
    CACHE BOOL
    "force-enable/-disable OSC's runtime assertions - even if building a release build"
)
set(
    OSC_FORCE_UNDEFINE_NDEBUG OFF
    CACHE BOOL
    "force-enable/disable NDEBUG - even if building a release build. Note: unlike OSC_FORCE_ASSERTS_ENABLED, this also tends to enable runtime assertions from library headers"
)
set(OSC_RUNTIME_PERF_MEASUREMENTS_ENABLED ON
    CACHE BOOL
    "enable/disable whether performance counters are collected at runtime wherever the `OSC_PERF` macro is used in the source code"
)

# -------------- set globally-applicable policies ---------------- #

# tell cmake that we will define MSVC warning flags (e.g. /W3 /W4) ourselves
cmake_policy(SET CMP0092 NEW)

# handle OSC_FORCE_UNDEFINE_NDEBUG
#
# this unsets the relevant flags globally
if(${OSC_FORCE_UNDEFINE_NDEBUG})
    # MSVC
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
    # others
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
endif()


# oscar_compiler_configuration:
#
# a cmake target that downstream projects can link to in order to inherit the
# configured compiler parameters (e.g. warning flags, language level, etc.) for
# oscar
add_library(oscar_compiler_configuration INTERFACE)
target_compile_options(oscar_compiler_configuration INTERFACE

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>:

        # set the warning level very high
        /W4

        # treat all warnings as errors
        /WX

        # keep frame pointers around, so that runtime stack traces can be dumped to error logs
        /Oy-

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
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:

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

        # disabled: requires newer gcc
        # warn if calls to `strcmp` and `strncmp` are determined to be invalid at compile-time
        # -Wstring-compare

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

        # disabled: doesn't work in some contexts where forward declarations are necessary
        # -Wredundant-decls

        # regardless of debug/release, pin the frame pointer register
        # so that stack traces are sane when debugging (even in Release).
        #
        # This adds some overhead (pins one register and requires callers
        # to setup their base pointers etc.) but makes debugging + profiling
        # the application much easier, even in release mode
        -fno-omit-frame-pointer
    >

    # clang flags
    $<$<CXX_COMPILER_ID:Clang>:
        # required in earlier clangs. Just setting
        # -fno-omit-frame-pointer (above) is not enough
        #
        # see:
        #   - https://stackoverflow.com/questions/43864881/fno-omit-frame-pointer-equivalent-compiler-option-for-clang
        #   - fixed here: https://reviews.llvm.org/D64294
        -mno-omit-leaf-frame-pointer

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
    >
)
target_compile_features(oscar_compiler_configuration INTERFACE cxx_std_20)

target_compile_definitions(oscar_compiler_configuration INTERFACE
    $<$<BOOL:${OSC_FORCE_ASSERTS_ENABLED}>:OSC_FORCE_ASSERTS_ENABLED>
    $<$<BOOL:${OSC_RUNTIME_PERF_MEASUREMENTS_ENABLED}>:OSC_RUNTIME_PERF_MEASUREMENTS_ENABLED>
)
