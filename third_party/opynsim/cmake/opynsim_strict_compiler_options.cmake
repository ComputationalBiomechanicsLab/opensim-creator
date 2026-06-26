# Adds a set of compiler flags to `target` that compile it with
# stricter rules/linter's than the compiler's default.
function(opyn_add_strict_compiler_options_to target)
    target_compile_options(${target} PRIVATE
        # msvc (Windows) flags
        $<$<CXX_COMPILER_ID:MSVC>:
            /WX                 # treat all warnings as errors
            /W4                 # set the warning level very high
            /external:W0        # disable warnings for external headers
            /analyze            # enable static code analysis
            /analyze:plugin EspXEngine.dll  # enable extended static analysis checks
            /analyze:external-  # disable static code analysis for external code
            /wd28020            # disable a bounds check (/analyze sucks at it)
            /wd6326             # disable noticing constant comparisons (/analyze mis-characterizes test suites)
            /wd26445            # disable requiring gsl::span
            /wd26440            # disable requiring noexcept
            /wd26447            # disable detecting noexcept violations
            /wd26446            # disable strict bounds checks
            /wd26481            # disable disallowing raw pointer arithmetic
            /wd26472            # disable specific arithmetic conversion checks
            /wd26485            # disable "no array to pointer decay" checks
            /wd26475            # disable disallowing function-style casts
            /wd26467            # disable narrowing checks
            /wd26455            # disable requiring noexcept default constructors
            /wd26821            # disable requiring gsl::span over std::span for bounds checks
            /wd26482            # disable requiring constant array access expressions
            /wd26459            # disable std::uninitialized_copy bounds check
            /wd26429            # disable null warning
            /wd26460            # disable const-checking arguments
            /wd26490            # disable reinterpret_cast warning
            /wd26409            # disable disallowing new/delete
            /wd26135            # disable lock annotation check
            /wd26860            # disable noticing that std::optional<T>::value can throw
            /wd26110            # disable faulty lock guard lifetime checks
            /wd26451            # disable arithmetic overflow checks
            /wd26496            # disable const-after-construction checks
            /wd26814            # disable "can be constexpr" checks
            /wd26417            # disable shared pointer reference param checks
            /wd26826            # disable checking for C-style varargs (currently necessary)
            /wd26400            # disable a gsl::owner<T> check when allocating
            /wd26497            # disable constexpr check
            /wd26813            # disable "use 'bitwise and' to check flag"
            /wd26474            # disable "don't cast when it can be implicit"
            /wd26402            # disable "return a scoped object instead of a heap-allocated one if has a move constructor"
            /wd26457            # disable "void should not be used to ignore values"
            /wd26491            # disable "don't use static_cast downcasts"
            /wd26466            # disable "don't use static_cast downcasts"
            /wd26493            # disable "don't use C-style casts"
            /wd26831            # disable "allocation size might be the result of numerical overflow"
            /wd26461            # disable "pointer can be marked 'const'"
            /wd26473            # disable "do not cast between the same pointer types"
            /wd26401            # disable "do not delete a raw pointer that is not an owner<T>"
            /wd26403            # disable "reset or explicitly delete an owner<T> pointer"
            /wd26426            # disable "global initializer calls a non-constexpr function"
            /wd26818            # disable "switch statement does not cover all cases" (triggered by googletest macros)
            /wd26414            # disable: "move, copy, reassign or reset a local smart pointer"
            /wd26820            # disable: "this is a potentially expensive copy operation"
            /wd26415            # disable: "smart pointer parameter is used only to access contained pointer"
            /wd26418            # disable: "shared pointer parameter is not copied or moved"
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
    )
endfunction()
