#pragma once

#include <oscar/Utils/FilenameExtractor.hpp>

#include <string_view>

namespace osc
{
    // calls into (hidden) assertion-handling implementation
    [[noreturn]] void OnAssertionFailure(
        std::string_view failingCode,
        std::string_view func,
        std::string_view file,
        unsigned int line
    );
}

// always execute this assertion - even if in release mode /w debug flags disabled
#define OSC_ASSERT_ALWAYS(expr)                                                                                       \
    (static_cast<bool>(expr) ? static_cast<void>(0) : osc::OnAssertionFailure(#expr, static_cast<char const*>(__func__), osc::ExtractFilename(__FILE__), __LINE__))

#ifdef OSC_FORCE_ASSERTS_ENABLED
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#elif !defined(NDEBUG)
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#else
#define OSC_ASSERT(expr)
#endif
