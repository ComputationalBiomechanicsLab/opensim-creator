#pragma once

#include <oscar/Platform/Event.h>
#include <oscar/UI/Panels/Panel.h>

#include <memory>
#include <utility>

namespace osc
{
    class OpenPanelEvent final : public Event {
    public:
        explicit OpenPanelEvent(std::unique_ptr<Panel> panel_to_open) :
            panel_to_open_{std::move(panel_to_open)}
        {
            enable_propagation();
        }

        bool has_tab() const { return panel_to_open_ != nullptr; }
        std::unique_ptr<Panel> take_panel() { return std::move(panel_to_open_); }
    private:
        std::unique_ptr<Panel> panel_to_open_;
    };
}
