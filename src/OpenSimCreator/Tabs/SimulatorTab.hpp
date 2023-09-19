#pragma once

#include <oscar/UI/Tabs/Tab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <SDL_events.h>

#include <memory>

namespace osc { class MainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class Simulation; }

namespace osc
{
    class SimulatorTab final : public Tab {
    public:
        SimulatorTab(
            ParentPtr<MainUIStateAPI> const&,
            std::shared_ptr<Simulation>
        );
        SimulatorTab(SimulatorTab const&) = delete;
        SimulatorTab(SimulatorTab&&) noexcept;
        SimulatorTab& operator=(SimulatorTab const&) = delete;
        SimulatorTab& operator=(SimulatorTab&&) noexcept;
        ~SimulatorTab() noexcept override;

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
