#pragma once

#include <libopensimcreator/Documents/OutputExtractors/OutputExtractor.h>

#include <memory>

namespace osc { class ISimulatorUIAPI; }

namespace osc
{
    class SimulationOutputPlot final {
    public:
        explicit SimulationOutputPlot(
            ISimulatorUIAPI*,
            OutputExtractor,
            float height
        );
        SimulationOutputPlot(const SimulationOutputPlot&) = delete;
        SimulationOutputPlot(SimulationOutputPlot&&) noexcept;
        SimulationOutputPlot& operator=(const SimulationOutputPlot&) = delete;
        SimulationOutputPlot& operator=(SimulationOutputPlot&&) noexcept;
        ~SimulationOutputPlot() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
