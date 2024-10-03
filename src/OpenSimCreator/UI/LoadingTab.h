#pragma once

#include <oscar/UI/Tabs/Tab.h>

#include <filesystem>

namespace osc { class Widget; }

namespace osc
{
    class LoadingTab final : public Tab {
    public:
        LoadingTab(Widget&, std::filesystem::path);

        bool isFinishedLoading() const;
    private:
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
