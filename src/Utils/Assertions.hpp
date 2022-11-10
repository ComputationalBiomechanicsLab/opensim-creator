#pragma once

#include "src/Utils/Macros.hpp"

namespace osc
{
    // calls into (hidden) assertion-handling implementation
    [[noreturn]] void OnAssertionFailure(char const* failingCode,
                                         char const* func,
                                         char const* file,
                                         unsigned int line) noexcept;

    // calls into (hidden) throwing-assertion implementation
    [[noreturn]] void OnThrowingAssertionFailure(char const* failingCode,
        char const* func,
        char const* file,
        unsigned int line);
}

#define OSC_THROWING_ASSERT(expr) \
    (static_cast<bool>(expr) ? (void)0 : osc::OnThrowingAssertionFailure(#expr, __func__, OSC_FILENAME, __LINE__))

// always execute this assertion - even if in release mode /w debug flags disabled
#define OSC_ASSERT_ALWAYS(expr)                                                                                       \
    (static_cast<bool>(expr) ? (void)0 : osc::OnAssertionFailure(#expr, __func__, OSC_FILENAME, __LINE__))

#ifdef OSC_FORCE_ASSERTS_ENABLED
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#elif !defined(NDEBUG)
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#else
#define OSC_ASSERT(expr)
#endif
