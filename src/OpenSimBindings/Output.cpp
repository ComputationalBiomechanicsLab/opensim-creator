#include "Output.hpp"

#include <iostream>
#include <sstream>
#include <string>

std::ostream& osc::operator<<(std::ostream& o, Output const& out)
{
    return o << "Output(id = " << out.getID() << ", " << out.getName() << ')';
}

std::string osc::to_string(Output const& out)
{
    std::stringstream ss;
    ss << out;
    return std::move(ss).str();
}
