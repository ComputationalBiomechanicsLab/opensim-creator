#pragma once

#include <format>
#include <exception>
#include <stdexcept>
#include <string>

namespace osc
{
    std::string potentially_nested_exception_to_string(const std::exception&, int indent = 0);

    template<typename... Args>
    std::runtime_error formatted_runtime_error(std::format_string<Args...> fmt, Args&&... args)
    {
        return std::runtime_error{std::format(fmt, std::forward<Args>(args)...)};
    }
}
