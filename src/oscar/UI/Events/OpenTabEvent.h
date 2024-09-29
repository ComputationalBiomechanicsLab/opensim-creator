#pragma once

#include <oscar/Platform/Event.h>
#include <oscar/UI/Tabs/Tab.h>

#include <concepts>
#include <memory>
#include <utility>

namespace osc
{
    class OpenTabEvent final : public Event {
    public:
        explicit OpenTabEvent(std::unique_ptr<Tab> tab_to_open) :
            tab_to_open_{std::move(tab_to_open)}
        {
            enable_propagation();
        }

        bool has_tab() const { return tab_to_open_ != nullptr; }
        std::unique_ptr<Tab> take_tab() { return std::move(tab_to_open_); }
    private:
        std::unique_ptr<Tab> tab_to_open_;
    };
}
