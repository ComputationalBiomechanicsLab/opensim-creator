#include "CStringView.hpp"

#include <iostream>
#include <string>
#include <string_view>

std::ostream& osc::operator<<(std::ostream& o, CStringView const& sv)
{
    return o << std::string_view{sv};
}

std::string osc::operator+(char const* c, CStringView const& sv)
{
    return c + to_string(sv);
}

std::string osc::operator+(std::string const& s, CStringView const& sv)
{
    return s + to_string(sv);
}
