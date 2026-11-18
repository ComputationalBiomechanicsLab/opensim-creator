#pragma once

// NOLINTBEGIN(readability-*)

namespace osc::cpp23
{
    template<typename O, typename T>
    struct out_value_result {
        O out;
        T value;
    };
}

// NOLINTEND(readability-*)
