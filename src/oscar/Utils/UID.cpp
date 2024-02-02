#include "UID.hpp"

#include <atomic>
#include <ostream>

using osc::UID;

constinit std::atomic<UID::element_type> osc::UID::g_NextID = 1;

std::ostream& osc::operator<<(std::ostream& o, UID const& id)
{
    return o << id.get();
}
