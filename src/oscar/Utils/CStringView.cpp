#include "CStringView.h"

#include <iostream>
#include <string>
#include <string_view>

std::ostream& osc::operator<<(std::ostream& o, const CStringView& sv)
{
    return o << std::string_view{sv};
}

std::string osc::operator+(const char* lhs, const CStringView& rhs)
{
    return lhs + to_string(rhs);
}

std::string osc::operator+(const std::string& lhs, const CStringView& rhs)
{
    return lhs + to_string(rhs);
}
