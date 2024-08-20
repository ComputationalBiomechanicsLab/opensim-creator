#include "AntiAliasingLevel.h"

#include <cstddef>
#include <ostream>

std::ostream& osc::operator<<(std::ostream& out, AntiAliasingLevel aa_level)
{
    return out << aa_level.get_as<uint32_t>() << 'x';
}
