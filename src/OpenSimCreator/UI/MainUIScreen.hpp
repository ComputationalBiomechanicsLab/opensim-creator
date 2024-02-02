#pragma once

#include <SDL_events.h>
#include <oscar/Platform/IScreen.hpp>
#include <oscar/Utils/UID.hpp>

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
        void implOnMount() final;
        void implOnUnmount() final;
        void implOnEvent(SDL_Event const&) final;
        void implOnTick() final;
        void implOnDraw() final;

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };
}
