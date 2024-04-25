#include "UID.h"

#include <atomic>
#include <ostream>

using namespace osc;

constinit std::atomic<UID::element_type> osc::UID::g_NextID = 1;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

std::ostream& osc::operator<<(std::ostream& o, const UID& id)
{
    return o << id.get();
}
