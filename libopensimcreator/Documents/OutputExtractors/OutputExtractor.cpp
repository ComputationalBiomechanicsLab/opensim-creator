#include "OutputExtractor.h"

#include <iostream>
#include <sstream>
#include <string>

std::ostream& osc::operator<<(std::ostream& o, const OutputExtractor& out)
{
    return o << "OutputExtractor(name = " << out.getName() << ')';
}

std::string osc::to_string(const OutputExtractor& out)
{
    std::stringstream ss;
    ss << out;
    return std::move(ss).str();
}
