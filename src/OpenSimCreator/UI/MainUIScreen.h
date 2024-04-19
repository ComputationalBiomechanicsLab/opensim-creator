#pragma once

#include <SDL_events.h>
#include <oscar/Platform/IScreen.h>
#include <oscar/Utils/UID.h>

#include <filesystem>
#include <memory>

namespace osc { class ITab; }
namespace osc { class ITabHost; }

namespace osc
{
    class MainUIScreen final : public IScreen {
    public:
        MainUIScreen();
        MainUIScreen(MainUIScreen const&) = delete;
        MainUIScreen(MainUIScreen&&) noexcept;
        MainUIScreen& operator=(MainUIScreen const&) = delete;
        MainUIScreen& operator=(MainUIScreen&&) noexcept;
        ~MainUIScreen() noexcept override;

        UID addTab(std::unique_ptr<ITab>);
        void open(std::filesystem::path const&);

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        void impl_on_event(SDL_Event const&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };
}
