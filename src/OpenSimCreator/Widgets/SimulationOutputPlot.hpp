#pragma once

#include "OpenSimCreator/OutputExtractor.hpp"

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

        void draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };

    // returns empty path if not saved
    std::filesystem::path TryPromptAndSaveOutputsAsCSV(SimulatorUIAPI&, nonstd::span<OutputExtractor const>);
    std::filesystem::path TryPromptAndSaveAllUserDesiredOutputsAsCSV(SimulatorUIAPI&);
}
