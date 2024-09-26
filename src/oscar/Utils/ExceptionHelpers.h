#pragma once

#include <exception>
#include <string>

namespace osc
{
    std::string potentially_nested_exception_to_string(const std::exception&);
}
