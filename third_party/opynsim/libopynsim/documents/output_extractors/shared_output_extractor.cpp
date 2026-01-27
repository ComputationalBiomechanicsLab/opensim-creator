#include "shared_output_extractor.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace opyn;

std::ostream& opyn::operator<<(std::ostream& o, const SharedOutputExtractor& out)
{
    return o << "SharedOutputExtractor(name = " << out.getName() << ')';
}

std::string opyn::to_string(const SharedOutputExtractor& out)
{
    std::stringstream ss;
    ss << out;
    return std::move(ss).str();
}
