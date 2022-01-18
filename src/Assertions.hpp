#pragma once

#include <cstddef>

namespace
{
    // compute the offset of the start of the filename from a full filepath (e.g. from __FILE__)
    //
    // e.g. "dir1/dir2/file.cpp" -> 10
    template<typename T, size_t S>
    inline constexpr size_t filenameOffset(const T(&str)[S], size_t i = S - 1) noexcept
    {
        if (i == 0)
        {
            return 0;
        }

        if (str[i] == '/' || str[i] == '\\')
        {
            return i + 1;
        }

        return filenameOffset(str, i - 1);
    }

    // try to guarantee that a value is computed at compile-time into an instance of this struct
    template<typename T, T v>
    struct ConstExprValue final {
        static constexpr const T value = v;
    };
}

namespace osc
{
    // calls into (hidden) assertion-handling implementation
    [[noreturn]] void onAssertionFailure(char const* failingCode,
                                         char const* func,
                                         char const* file,
                                         unsigned int line) noexcept;
}

// macro that produces just the filename (rather than the full filepath)
#define OSC_FILENAME &__FILE__[::ConstExprValue<size_t, filenameOffset(__FILE__)>::value]

// always execute this assertion - even if in release mode /w debug flags disabled
#define OSC_ASSERT_ALWAYS(expr)                                                                                       \
    (static_cast<bool>(expr) ? (void)0 : osc::onAssertionFailure(#expr, __func__, OSC_FILENAME, __LINE__))

#ifdef OSC_FORCE_ASSERTS_ENABLED
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#elif !defined(NDEBUG)
#define OSC_ASSERT(expr) OSC_ASSERT_ALWAYS(expr)
#else
#define OSC_ASSERT(expr)
#endif
