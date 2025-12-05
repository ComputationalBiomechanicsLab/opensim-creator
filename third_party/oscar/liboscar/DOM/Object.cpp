#include "Object.h"

#include <ostream>

std::ostream& osc::operator<<(std::ostream& o, const Object& obj)
{
    return o << obj.to_string();
}
