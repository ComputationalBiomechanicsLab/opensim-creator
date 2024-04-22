#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class Simulation; }

namespace osc
{
    class SimulationTab final : public ITab {
    public:
        SimulationTab(
            ParentPtr<IMainUIStateAPI> const&,
            std::shared_ptr<Simulation>
        );
        SimulationTab(SimulationTab const&) = delete;
        SimulationTab(SimulationTab&&) noexcept;
        SimulationTab& operator=(SimulationTab const&) = delete;
        SimulationTab& operator=(SimulationTab&&) noexcept;
        ~SimulationTab() noexcept override;

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
