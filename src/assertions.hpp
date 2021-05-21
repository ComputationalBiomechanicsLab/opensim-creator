#pragma once

namespace osc {
    [[noreturn]] void on_assertion_failure(char const* failing_code, char const* file, unsigned int line) noexcept;
}

// always execute this assertion - even if in release mode /w debug flags disabled
#define OSC_ASSERT_ALWAYS(expr)                                                                                       \
    (static_cast<bool>(expr) ? (void)0 : osc::on_assertion_failure(#expr, __FILE__, __LINE__))

#ifdef OSC_FORCE_ASSERTS_ENABLED
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#elif !defined(NDEBUG)
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#else
#define OSC_ASSERT(expr)
#endif
