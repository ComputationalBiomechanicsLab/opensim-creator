#pragma once

#include <oscar/UI/Tabs/Tab.h>

#include <filesystem>

namespace osc { class MainUIScreen; }

namespace osc
{
    class LoadingTab final : public Tab {
    public:
        LoadingTab(MainUIScreen&, std::filesystem::path);

    private:
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
