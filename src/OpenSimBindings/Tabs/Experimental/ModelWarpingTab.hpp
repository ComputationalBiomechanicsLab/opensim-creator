#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class ModelWarpingTab final : public Tab {
    public:
        static CStringView id() noexcept;

        ModelWarpingTab(TabHost*);
        ModelWarpingTab(ModelWarpingTab const&) = delete;
        ModelWarpingTab(ModelWarpingTab&&) noexcept = default;
        ModelWarpingTab& operator=(ModelWarpingTab const&) = delete;
        ModelWarpingTab& operator=(ModelWarpingTab&&) noexcept = default;
        ~ModelWarpingTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
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