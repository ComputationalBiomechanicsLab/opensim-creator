#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc { class Simulation; }
namespace osc { class ISimulatorUIAPI; }

namespace osc
{
    class SimulationDetailsPanel final : public IPanel {
    public:
        SimulationDetailsPanel(
            std::string_view panelName,
            ISimulatorUIAPI*,
            std::shared_ptr<Simulation const>
        );
        SimulationDetailsPanel(SimulationDetailsPanel const&) = delete;
        SimulationDetailsPanel(SimulationDetailsPanel&&) noexcept;
        SimulationDetailsPanel& operator=(SimulationDetailsPanel const&) = delete;
        SimulationDetailsPanel& operator=(SimulationDetailsPanel&&) noexcept;
        ~SimulationDetailsPanel() noexcept;

    private:
        CStringView implGetName() const final;
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
