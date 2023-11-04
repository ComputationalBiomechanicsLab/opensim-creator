#pragma once

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/Macros.hpp>

namespace osc
{
    // calls into (hidden) assertion-handling implementation
    [[noreturn]] void OnAssertionFailure(
        CStringView failingCode,
        CStringView func,
        CStringView file,
        unsigned int line
    );
}

// always execute this assertion - even if in release mode /w debug flags disabled
#define OSC_ASSERT_ALWAYS(expr)                                                                                       \
    (static_cast<bool>(expr) ? static_cast<void>(0) : osc::OnAssertionFailure(#expr, osc::CStringView::FromArray(__func__), OSC_FILENAME, __LINE__))

#ifdef OSC_FORCE_ASSERTS_ENABLED
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#elif !defined(NDEBUG)
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#else
#define OSC_ASSERT(expr)
#endif
