#pragma once

#include <liboscar/platform/events/event.h>
#include <OpenSim/Common/ComponentPath.h>

#include <utility>

namespace osc
{
    class OpenComponentContextMenuEvent : public Event {
    public:
        explicit OpenComponentContextMenuEvent(OpenSim::ComponentPath path) :
            component_path_{std::move(path)}
        {
            enable_propagation();
        }

        const OpenSim::ComponentPath& path() const { return component_path_; }

    private:
        OpenSim::ComponentPath component_path_;
    };
}
