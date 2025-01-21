#pragma once

#include <liboscar/Platform/Events/Event.h>
#include <OpenSim/Common/ComponentPath.h>

#include <utility>

namespace osc
{
    class OpenComponentContextMenuEvent : public Event {
    public:
        explicit OpenComponentContextMenuEvent(OpenSim::ComponentPath path) :
            m_ComponentPath{std::move(path)}
        {
            enable_propagation();
        }

        const OpenSim::ComponentPath& path() const { return m_ComponentPath; }
    private:
        OpenSim::ComponentPath m_ComponentPath;
    };
}
