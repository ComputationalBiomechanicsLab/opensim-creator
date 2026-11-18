#pragma once

#include <liboscar/ui/tabs/tab.h>

#include <filesystem>

namespace osc
{
    class Widget;

    class LoadingTab final : public Tab {
    public:
        explicit LoadingTab(Widget* parent, std::filesystem::path);

        bool is_finished_loading() const;

    private:
        void impl_on_tick() override;
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
