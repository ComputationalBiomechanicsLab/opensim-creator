#include "symbol.h"

#include <ostream>

std::ostream& opyn::operator<<(std::ostream& ostream, const Symbol& symbol)
{
    return ostream << "Symbol(name=\"" << symbol.name() << "\")";
}
