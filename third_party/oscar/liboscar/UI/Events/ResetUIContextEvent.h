#pragma once

#include <liboscar/Platform/Events/Event.h>

namespace osc
{
    class ResetUIContextEvent final : public Event {
    public:
        ResetUIContextEvent() { enable_propagation(); }
    };
}
