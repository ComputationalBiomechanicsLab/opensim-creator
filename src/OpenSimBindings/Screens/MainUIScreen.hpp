#pragma once

#include "src/Screens/Screen.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace osc { class Tab; }
namespace osc { class TabHost; }

namespace osc
{
    class MainUIScreen final : public Screen {
    public:
        MainUIScreen();
        MainUIScreen(std::vector<std::filesystem::path> const&);
        MainUIScreen(MainUIScreen const&) = delete;
        MainUIScreen(MainUIScreen&&) noexcept;
        MainUIScreen& operator=(MainUIScreen const&) = delete;
        MainUIScreen& operator=(MainUIScreen&&) noexcept;
        ~MainUIScreen() noexcept override;

        UID addTab(std::unique_ptr<Tab>);
        TabHost* getTabHostAPI();

    private:
        void implOnMount() final;
        void implOnUnmount() final;
        void implOnEvent(SDL_Event const&) final;
        void implOnTick() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
