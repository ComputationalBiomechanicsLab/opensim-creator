#pragma once

#include <OpenSimCreator/OutputExtractors/OutputExtractor.hpp>

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

    // returns empty path if not saved
    std::filesystem::path TryPromptAndSaveOutputsAsCSV(ISimulatorUIAPI&, std::span<OutputExtractor const>);
    std::filesystem::path TryPromptAndSaveAllUserDesiredOutputsAsCSV(ISimulatorUIAPI&);
}
