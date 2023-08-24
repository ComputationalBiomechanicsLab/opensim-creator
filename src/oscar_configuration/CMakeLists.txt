# define MSVC warning flags (e.g. /W3 /W4) ourselves
cmake_policy(SET CMP0092 NEW)

# OscarConfiguration.hpp
#
# a configured header file that contains values that are accessible in the source
# code at compile-time
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/OscarConfiguration.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/OscarConfiguration.hpp"
)

# oscar_configuration:
#
# a cmake target that downstream projects can link to in order to inherit the
# configured compiler parameters (e.g. warning flags, language level, etc.) for
# oscar
add_library(oscar_configuration INTERFACE)
target_compile_options(oscar_configuration INTERFACE

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>:

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

        # if requested, enable MSVC's libASAN
        $<$<BOOL:${OSC_USE_ASAN}>:/fsanitize=address>
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

        # warn if using an uninitialized variable
        -Wuninitialized

        # warn if an uninitialized variable is initialized by itself
        -Winit-self

        # warn if a variable's name shadows an earlier (higher scope) variable with the same name
        -Wshadow

        # warn if data is casted to a higher alignment (e.g. char -> int64_t)
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

        # disabled: broken in osc
        # warn if an implicit conversion may alter a value
        # -Wconversion

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

        # if address sanitizer is specified, use it
        $<$<BOOL:${OSC_USE_ASAN}>:-fsanitize=address>
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
    >

    # gcc flags
    $<$<CXX_COMPILER_ID:GNU>:
    >
)
target_link_options(oscar_configuration INTERFACE

    # Linux /w GCC or Clang
    $<$<AND:$<BOOL:LINUX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:

        # make `ld` emit symbols for all functions - even if they aren't
        # externally used (with an exe, only `main` is used, really)
        #
        # this makes the binary a little bigger (~400 KB) but means
        # that the application can emit its own stack trace /w function
        # names - even in Release mode.
        -rdynamic
    >

    # GCC or Clang: handle OSC_USE_ASAN
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        $<$<BOOL:${OSC_USE_ASAN}>:-fsanitize=address>
    >
)
target_include_directories(oscar_configuration INTERFACE
    # so that source code can `#include <OscarConfiguration.hpp>`
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"
)
target_compile_features(oscar_configuration INTERFACE cxx_std_17)

target_link_libraries(oscar_configuration INTERFACE

    # in earlier gcc/clang the <filesystem> implementation must be explicitly linked
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>
)

# handle OSC_FORCE_ASSERTS_ENABLED
if(OSC_FORCE_ASSERTS_ENABLED)
    target_compile_definitions(oscar_configuration INTERFACE -DOSC_FORCE_ASSERTS_ENABLED=1)
endif()
