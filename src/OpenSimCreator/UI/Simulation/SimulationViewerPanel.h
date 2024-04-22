#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc { class SimulationViewerPanelParameters; }

namespace osc
{
    class SimulationViewerPanel final : public IPanel {
    public:
        SimulationViewerPanel(
            std::string_view panelName,
            SimulationViewerPanelParameters
        );
        SimulationViewerPanel(SimulationViewerPanel const&) = delete;
        SimulationViewerPanel(SimulationViewerPanel&&) noexcept;
        SimulationViewerPanel& operator=(SimulationViewerPanel const&) = delete;
        SimulationViewerPanel& operator=(SimulationViewerPanel&&) noexcept;
        ~SimulationViewerPanel() noexcept;

    private:
        CStringView impl_get_name() const final;
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
