#include "UID.hpp"

#include <atomic>
#include <ostream>

int64_t osc::UID::GetNextID()
{
    static std::atomic<int64_t> s_NextID = 1;
    return s_NextID.fetch_add(1, std::memory_order_relaxed);
}

std::ostream& osc::operator<<(std::ostream& o, UID const& id)
{
    return o << id.get();
}
