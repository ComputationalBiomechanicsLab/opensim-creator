#pragma once

#include <liboscar/platform/events/Event.h>

namespace osc
{
    class ResetUIContextEvent final : public Event {
    public:
        ResetUIContextEvent() { enable_propagation(); }
    };
}
