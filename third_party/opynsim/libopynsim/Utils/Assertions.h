#pragma once

#include <libopynsim/opynsimconfig.h>

#include <cstddef>
#include <string_view>

namespace opyn::detail
{
    template<size_t N>
    consteval std::string_view extract_filename(const char(&p)[N])
    {
        for (auto i = static_cast<ptrdiff_t>(N) - 2; i > 0; --i) {
            if (p[i] == '/' or p[i] == '\\') {
                return std::string_view{p + i + 1, static_cast<size_t>((static_cast<ptrdiff_t>(N) - 2) - i)};
            }
        }
        return {p, N-1};  // else: no slashes, return as-is
    }

    // calls into (hidden) assertion-handling implementation
    [[noreturn]] void on_assertion_failure(
        std::string_view failing_code,
        std::string_view function_name,
        std::string_view file_name,
        unsigned int file_line
    );
}

// always execute this assertion - even if in release mode /w debug flags disabled
#define OPYN_ASSERT_ALWAYS(expr) \
    (static_cast<bool>(expr) ? static_cast<void>(0) : opyn::detail::on_assertion_failure(#expr, static_cast<const char*>(__func__), opyn::detail::extract_filename(__FILE__), __LINE__))

#if OPYN_FORCE_ASSERTS_ENABLED
    #define OPYN_ASSERT(expr) OPYN_ASSERT_ALWAYS(expr)
#elif !defined(NDEBUG)
    #define OPYN_ASSERT(expr) OPYN_ASSERT_ALWAYS(expr)
#else
    #define OPYN_ASSERT(expr)
#endif
