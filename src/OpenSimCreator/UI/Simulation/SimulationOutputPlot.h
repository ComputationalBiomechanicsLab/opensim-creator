#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>

#include <filesystem>
#include <memory>
#include <span>

namespace osc { class ISimulatorUIAPI; }

namespace osc
{
    class SimulationOutputPlot final {
    public:
        SimulationOutputPlot(
            ISimulatorUIAPI*,
            OutputExtractor,
            float height
        );
        SimulationOutputPlot(SimulationOutputPlot const&) = delete;
        SimulationOutputPlot(SimulationOutputPlot&&) noexcept;
        SimulationOutputPlot& operator=(SimulationOutputPlot const&) = delete;
        SimulationOutputPlot& operator=(SimulationOutputPlot&&) noexcept;
        ~SimulationOutputPlot() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
