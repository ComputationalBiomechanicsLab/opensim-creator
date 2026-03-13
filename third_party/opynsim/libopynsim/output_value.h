#pragma once

#include <string>
#include <variant>

namespace opyn
{
    using OutputValue = std::variant<int, float, std::string>;
}
