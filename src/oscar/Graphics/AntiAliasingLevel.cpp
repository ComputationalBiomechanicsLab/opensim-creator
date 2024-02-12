#include "AntiAliasingLevel.h"

#include <iostream>
#include <sstream>
#include <string>
#include <utility>

std::ostream& osc::operator<<(std::ostream& o, AntiAliasingLevel level)
{
    return o << level.getU32() << 'x';
}

std::string osc::to_string(AntiAliasingLevel level)
{
    std::stringstream ss;
    ss << level;
    return std::move(ss).str();
}
