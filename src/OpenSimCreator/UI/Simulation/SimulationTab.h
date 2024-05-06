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
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(SDL_Event const&) final;
        void impl_on_tick() final;
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
