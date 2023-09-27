#pragma once

#include <string>
#include <variant>

namespace osc
{
    using AppSettingValue = std::variant<
        std::string
    >;
}
