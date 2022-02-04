#include "UID.hpp"

#include <iostream>

std::atomic<int64_t> osc::g_NextGlobalUID = 1;
osc::UID osc::g_EmptyID = osc::GenerateID();
osc::UID osc::g_InvalidID = osc::GenerateID();

std::ostream& osc::operator<<(std::ostream& o, UID const& id)
{
    return o << UnwrapID(id);
}
