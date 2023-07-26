#pragma once

#include "OpenSimCreator/Outputs/OutputExtractor.hpp"

#include <nonstd/span.hpp>

#include <filesystem>
#include <memory>

namespace osc { class SimulatorUIAPI; }

namespace osc
{
    class SimulationOutputPlot final {
    public:
        SimulationOutputPlot(SimulatorUIAPI*, OutputExtractor, float height);
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
    std::filesystem::path TryPromptAndSaveOutputsAsCSV(SimulatorUIAPI&, nonstd::span<OutputExtractor const>);
    std::filesystem::path TryPromptAndSaveAllUserDesiredOutputsAsCSV(SimulatorUIAPI&);
}
