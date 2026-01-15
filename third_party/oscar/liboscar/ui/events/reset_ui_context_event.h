#pragma once

#include <liboscar/platform/events/event.h>

namespace osc
{
    class ResetUIContextEvent final : public Event {
    public:
        ResetUIContextEvent() { enable_propagation(); }
    };
}
