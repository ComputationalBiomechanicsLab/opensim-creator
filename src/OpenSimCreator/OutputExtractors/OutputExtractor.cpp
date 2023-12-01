#include "OutputExtractor.hpp"

#include <iostream>
#include <sstream>
#include <string>

std::ostream& osc::operator<<(std::ostream& o, OutputExtractor const& out)
{
    return o << "OutputExtractor(name = " << out.getName() << ')';
}

std::string osc::to_string(OutputExtractor const& out)
{
    std::stringstream ss;
    ss << out;
    return std::move(ss).str();
}
