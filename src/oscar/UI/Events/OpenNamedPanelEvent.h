#pragma once

#include <oscar/Platform/Event.h>

#include <string>
#include <string_view>

namespace osc
{
    class OpenNamedPanelEvent : public Event {
    public:
        explicit OpenNamedPanelEvent(std::string_view panel_name) :
            panel_name_{panel_name}
        {
            enable_propagation();
        }

        std::string_view panel_name() const { return panel_name_; }
    private:
        std::string panel_name_;
    };
}
