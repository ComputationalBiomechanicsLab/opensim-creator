#include "UID.hpp"

#include <iostream>

std::atomic<int64_t> osc::g_NextGlobalUID = 1;

std::ostream& osc::operator<<(std::ostream& o, UID const& id)
{
    return o << UnwrapID(id);
}
