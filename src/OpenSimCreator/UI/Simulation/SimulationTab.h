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
            const ParentPtr<IMainUIStateAPI>&,
            std::shared_ptr<Simulation>
        );
        SimulationTab(const SimulationTab&) = delete;
        SimulationTab(SimulationTab&&) noexcept;
        SimulationTab& operator=(const SimulationTab&) = delete;
        SimulationTab& operator=(SimulationTab&&) noexcept;
        ~SimulationTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(const SDL_Event&) final;
        void impl_on_tick() final;
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
