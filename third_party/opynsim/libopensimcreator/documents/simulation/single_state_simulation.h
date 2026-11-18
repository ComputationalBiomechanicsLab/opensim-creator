#pragma once

#include <libopensimcreator/documents/model/basic_model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/simulation/abstract_simulation.h>
#include <libopensimcreator/documents/simulation/simulation_clocks.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/documents/simulation/simulation_status.h>

#include <liboscar/utilities/synchronized_value_guard.h>

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace OpenSim
{
    class Model;
}

namespace osc
{
    class Environment;

    class SingleStateSimulation final : public AbstractSimulation {
    public:
        explicit SingleStateSimulation(BasicModelStatePairWithSharedEnvironment);
        SingleStateSimulation(const SingleStateSimulation&) = delete;
        SingleStateSimulation(SingleStateSimulation&&) noexcept;
        SingleStateSimulation& operator=(const SingleStateSimulation&) = delete;
        SingleStateSimulation& operator=(SingleStateSimulation&&) noexcept;
        ~SingleStateSimulation() noexcept override;

    private:
        SynchronizedValueGuard<const OpenSim::Model> impl_get_model() const override;

        ptrdiff_t impl_get_num_reports() const override;
        SimulationReport impl_get_simulation_report(ptrdiff_t) const override;
        std::vector<SimulationReport> impl_get_all_simulation_reports() const override;

        SimulationStatus impl_get_status() const override;
        SimulationClocks impl_get_clocks() const override;
        const ParamBlock& impl_get_params() const override;
        std::span<const opyn::SharedOutputExtractor> impl_get_output_extractors() const override;

        float impl_get_fixup_scale_factor() const override;
        void impl_set_fixup_scale_factor(float) override;

        std::shared_ptr<Environment> impl_upd_associated_environment() override;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
