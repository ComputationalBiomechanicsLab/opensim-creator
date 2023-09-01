#pragma once

#include <oscar/Screens/Screen.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <oscar/Utils/UID.hpp>

#include <SDL_events.h>

#include <filesystem>
#include <memory>

namespace osc { class Tab; }
namespace osc { class TabHost; }

namespace osc
{
    class MainUIScreen final : public Screen {
    public:
        MainUIScreen();
        MainUIScreen(MainUIScreen const&) = delete;
        MainUIScreen(MainUIScreen&&) noexcept;
        MainUIScreen& operator=(MainUIScreen const&) = delete;
        MainUIScreen& operator=(MainUIScreen&&) noexcept;
        ~MainUIScreen() noexcept override;

        UID addTab(std::unique_ptr<Tab>);
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
