#pragma once

#include <oscar/Platform/Event.h>
#include <oscar/Utils/UID.h>

namespace osc
{
    class SelectTabEvent final : Event {
    public:
        explicit SelectTabEvent(UID tab_to_select) :
            tab_to_select_{tab_to_select}
        {}

        UID tab_to_select() const { return tab_to_select_; }
    private:
        UID tab_to_select_;
    };
}
