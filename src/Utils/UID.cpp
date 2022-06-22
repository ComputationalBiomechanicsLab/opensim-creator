#include "UID.hpp"

#include <atomic>
#include <ostream>

int64_t osc::UID::GetNextID() noexcept
{
    static std::atomic<int64_t> g_NextID = 1;
    return g_NextID.fetch_add(1, std::memory_order_relaxed);
}

std::ostream& osc::operator<<(std::ostream& o, UID const& id)
{
    return o << id.get();
}
