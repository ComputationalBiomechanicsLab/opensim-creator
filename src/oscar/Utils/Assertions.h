#pragma once

#include <oscar/Utils/FilenameExtractor.h>

#include <string_view>

namespace osc::detail
{
    // calls into (hidden) assertion-handling implementation
    [[noreturn]] void on_assertion_failure(
        std::string_view failing_code,
        std::string_view function_name,
        std::string_view file_name,
        unsigned int file_line
    );
}

// always execute this assertion - even if in release mode /w debug flags disabled
#define OSC_ASSERT_ALWAYS(expr) \
    (static_cast<bool>(expr) ? static_cast<void>(0) : osc::detail::on_assertion_failure(#expr, static_cast<const char*>(__func__), osc::ExtractFilename(__FILE__), __LINE__))

#ifdef OSC_FORCE_ASSERTS_ENABLED
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#elif !defined(NDEBUG)
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#else
#define OSC_ASSERT(expr)
#endif
