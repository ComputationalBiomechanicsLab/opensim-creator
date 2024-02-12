#include "CStringView.h"

#include <iostream>
#include <string>
#include <string_view>

std::ostream& osc::operator<<(std::ostream& o, CStringView const& sv)
{
    return o << std::string_view{sv};
}

std::string osc::operator+(char const* lhs, CStringView const& rhs)
{
    return lhs + to_string(rhs);
}

std::string osc::operator+(std::string const& lhs, CStringView const& rhs)
{
    return lhs + to_string(rhs);
}
