#pragma once

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>

#include <memory>

namespace osc
{
    class SimulatorUIAPI;

    class SimulationOutputPlot final {
    public:
        explicit SimulationOutputPlot(SimulatorUIAPI*, opyn::SharedOutputExtractor, float height);
        SimulationOutputPlot(const SimulationOutputPlot&) = delete;
        SimulationOutputPlot(SimulationOutputPlot&&) noexcept;
        SimulationOutputPlot& operator=(const SimulationOutputPlot&) = delete;
        SimulationOutputPlot& operator=(SimulationOutputPlot&&) noexcept;
        ~SimulationOutputPlot() noexcept;

        void on_draw();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
