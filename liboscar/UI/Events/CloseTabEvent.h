#pragma once

#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Utils/UID.h>

namespace osc
{
    class CloseTabEvent final : public Event {
    public:
        explicit CloseTabEvent(UID tabid_to_close) :
            tabid_to_close_{tabid_to_close}
        {
            enable_propagation();
        }

        UID tabid_to_close() const { return tabid_to_close_; }
    private:
        UID tabid_to_close_;
    };
}
