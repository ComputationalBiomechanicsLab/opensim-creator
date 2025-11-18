#include "CStringView.h"

#include <iostream>
#include <string>
#include <string_view>

std::ostream& osc::operator<<(std::ostream& out, const CStringView& sv)
{
    return out << std::string_view{sv};
}

std::string osc::operator+(const char* lhs, const CStringView& rhs)
{
    return lhs + to_string(rhs);
}

std::string osc::operator+(const std::string& lhs, const CStringView& rhs)
{
    return lhs + to_string(rhs);
}
