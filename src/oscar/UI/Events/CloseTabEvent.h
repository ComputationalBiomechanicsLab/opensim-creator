#pragma once

#include <oscar/Platform/Event.h>
#include <oscar/Utils/UID.h>

namespace osc
{
    class CloseTabEvent final : public Event {
    public:
        explicit CloseTabEvent(UID tabid_to_close) :
            tabid_to_close_{tabid_to_close}
        {}

        UID tabid_to_close() const { return tabid_to_close_; }
    private:
        UID tabid_to_close_;
    };
}
