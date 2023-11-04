#pragma once

#include <source_location>
#include <string_view>

namespace osc
{
    // calls into (hidden) assertion-handling implementation
    [[noreturn]] void OnAssertionFailure(
        std::string_view failingCode,
        std::source_location
    );
}

// always execute this assertion - even if in release mode /w debug flags disabled
#define OSC_ASSERT_ALWAYS(expr)                                                                                       \
    (static_cast<bool>(expr) ? static_cast<void>(0) : osc::OnAssertionFailure(#expr, std::source_location::current()))

#ifdef OSC_FORCE_ASSERTS_ENABLED
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#elif !defined(NDEBUG)
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#else
#define OSC_ASSERT(expr)
#endif
