#pragma once

namespace
{
    // compute the offset of the start of the filename from a full filepath (e.g. from __FILE__)
    //
    // e.g. "dir1/dir2/file.cpp" -> 10
    template<typename T, int S>
    inline constexpr int filenameOffset(const T(&str)[S], int i = S - 1) noexcept
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
        static T constexpr value = v;
    };
}

// macro that produces just the filename (rather than the full filepath)
#define OSC_FILENAME &__FILE__[::ConstExprValue<int, filenameOffset(__FILE__)>::value]

// paste two tokens together (preprocessor hack)
#define OSC_TOKENPASTE(x, y) x##y
#define OSC_TOKENPASTE2(x, y) OSC_TOKENPASTE(x, y)
