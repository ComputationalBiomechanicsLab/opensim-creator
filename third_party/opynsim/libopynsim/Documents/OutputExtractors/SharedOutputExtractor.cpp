#include "SharedOutputExtractor.h"

#include <iostream>
#include <sstream>
#include <string>

std::ostream& osc::operator<<(std::ostream& o, const SharedOutputExtractor& out)
{
    return o << "SharedOutputExtractor(name = " << out.getName() << ')';
}

std::string osc::to_string(const SharedOutputExtractor& out)
{
    std::stringstream ss;
    ss << out;
    return std::move(ss).str();
}
