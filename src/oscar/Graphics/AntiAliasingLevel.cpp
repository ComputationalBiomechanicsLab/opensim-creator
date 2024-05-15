#include "AntiAliasingLevel.h"

#include <iostream>
#include <sstream>
#include <string>
#include <utility>

std::ostream& osc::operator<<(std::ostream& out, AntiAliasingLevel aa_level)
{
    return out << aa_level.get_as<uint32_t>() << 'x';
}

std::string osc::to_string(AntiAliasingLevel aa_level)
{
    std::stringstream ss;
    ss << aa_level;
    return std::move(ss).str();
}
