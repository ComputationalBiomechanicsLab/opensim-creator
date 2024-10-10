#pragma once

#include <oscar/Platform/Screen.h>

#include <filesystem>

namespace osc { class Event; }

namespace osc
{
    class MainUIScreen final : public Screen {
    public:
        MainUIScreen();
        MainUIScreen(const MainUIScreen&) = delete;
        MainUIScreen(MainUIScreen&&) noexcept = delete;
        MainUIScreen& operator=(const MainUIScreen&) = delete;
        MainUIScreen& operator=(MainUIScreen&&) noexcept = delete;
        ~MainUIScreen() noexcept override;

        void open(const std::filesystem::path&);

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
