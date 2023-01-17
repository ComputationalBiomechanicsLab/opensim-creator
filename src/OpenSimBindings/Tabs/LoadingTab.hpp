#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <filesystem>
#include <memory>

namespace osc { class MainUIStateAPI; }
namespace osc { class TabHost; }

namespace osc
{
    class LoadingTab final : public Tab {
    public:
        LoadingTab(MainUIStateAPI*, std::filesystem::path);
        LoadingTab(LoadingTab const&) = delete;
        LoadingTab(LoadingTab&&) noexcept;
        LoadingTab& operator=(LoadingTab const&) = delete;
        LoadingTab& operator=(LoadingTab&&) noexcept;
        ~LoadingTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        TabHost* implParent() const final;
        void implOnMount() final;
        void implOnUnmount() final;
        bool implOnEvent(SDL_Event const&) final;
        void implOnTick() final;
        void implOnDrawMainMenu() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
