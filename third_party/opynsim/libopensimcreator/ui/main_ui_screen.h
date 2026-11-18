#pragma once

#include <liboscar/platform/widget.h>

#include <filesystem>

namespace osc
{
    class Event;

    class MainUIScreen final : public Widget {
    public:
        MainUIScreen();
        MainUIScreen(const MainUIScreen&) = delete;
        MainUIScreen(MainUIScreen&&) noexcept = delete;
        MainUIScreen& operator=(const MainUIScreen&) = delete;
        MainUIScreen& operator=(MainUIScreen&&) noexcept = delete;
        ~MainUIScreen() noexcept override;

        void open(const std::filesystem::path&);

    private:
        void impl_on_mount() override;
        void impl_on_unmount() override;
        bool impl_on_event(Event&) override;
        void impl_on_tick() override;
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
