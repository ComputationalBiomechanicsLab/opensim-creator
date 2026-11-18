#include "sto_file_simulation.h"

#include <libopensimcreator/documents/simulation/simulation_clock.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/documents/simulation/simulation_status.h>
#include <libopensimcreator/documents/param_block.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/StateVector.h>
#include <OpenSim/Common/Storage.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    SimTK::State create_state_from_storage_row(
        OpenSim::Model& model,
        const OpenSim::Storage& storage,
        const std::unordered_map<int, int>& lut,
        int row)
    {
        SimTK::State state = model.getWorkingState();
        opyn::update_state_variables_from_storage_row(model, state, lut, storage, row);
        model.assemble(state);
        model.realizeReport(state);
        return state;
    }

    std::vector<SimulationReport> extract_reports(
        OpenSim::Model& model, const std::filesystem::path& sto_file_path)
    {
        // load+condition the underlying `OpenSim::Storage`, resample
        // the state trajectory at 100FPS (#708).
        const auto storage = opyn::load_storage(
            model,
            sto_file_path,
            opyn::StorageLoadingParameters{.resample_to_frequency = 1.0 / 100.0});

        // map column header indices in the `OpenSim::Storage` to the `OpenSim::Model`'s state
        // variables' indices
        const auto lut =
            opyn::create_storage_index_to_model_statevar_mapping_with_warnings(model, *storage);

        // init the model+state with unlocked coordinates
        opyn::initialize_model(model);
        opyn::initialize_state(model);

        // convert each timestep in the `OpenSim::Storage` as a `SimulationReport`
        std::vector<SimulationReport> rv;
        rv.reserve(storage->getSize());
        for (int row = 0; row < storage->getSize(); ++row) {
            rv.emplace_back(create_state_from_storage_row(model, *storage, lut, row));
        }
        return rv;
    }
}

class osc::StoFileSimulation::Impl final {
public:
    Impl(
        std::unique_ptr<OpenSim::Model> model,
        const std::filesystem::path& sto_file_path,
        float fixup_scale_factor,
        std::shared_ptr<Environment> environment) :

        model_{std::move(model)},
        simulation_reports_{extract_reports(*model_, sto_file_path)},
        fixup_scale_factor_{fixup_scale_factor},
        environment_{std::move(environment)}
    {}

    SynchronizedValueGuard<const OpenSim::Model> get_model() const
    {
        return {model_mutex_, *model_};
    }

    size_t get_num_reports() const { return simulation_reports_.size(); }

    SimulationReport get_simulation_report(ptrdiff_t report_index) const
    {
        return simulation_reports_.at(report_index);
    }

    std::vector<SimulationReport> get_all_simulation_reports() const { return simulation_reports_; }

    SimulationStatus get_status() const { return SimulationStatus::Completed; }

    SimulationClocks get_clocks() const { return SimulationClocks{{start_, end_}}; }

    const ParamBlock& get_params() const { return param_block_; }

    std::span<const opyn::SharedOutputExtractor> get_output_extractors() const { return {}; }

    float get_fixup_scale_factor() const { return fixup_scale_factor_; }

    void set_fixup_scale_factor(float v) { fixup_scale_factor_ = v; }

    std::shared_ptr<Environment> impl_upd_associated_environment() { return environment_; }

private:
    mutable std::mutex model_mutex_;
    std::unique_ptr<OpenSim::Model> model_;
    std::vector<SimulationReport> simulation_reports_;
    SimulationClock::time_point start_ = simulation_reports_.empty()
                                             ? SimulationClock::start()
                                             : simulation_reports_.front().get_time();
    SimulationClock::time_point end_ = simulation_reports_.empty()
                                           ? SimulationClock::start()
                                           : simulation_reports_.back().get_time();
    ParamBlock param_block_;
    float fixup_scale_factor_ = 1.0f;
    std::shared_ptr<Environment> environment_;
};

osc::StoFileSimulation::StoFileSimulation(
    std::unique_ptr<OpenSim::Model> model,
    const std::filesystem::path& sto_file_path,
    float fixup_scale_factor,
    std::shared_ptr<Environment> environment) :
    impl_{std::make_unique<Impl>(
        std::move(model), sto_file_path, fixup_scale_factor, std::move(environment))}
{}
osc::StoFileSimulation::StoFileSimulation(StoFileSimulation&&) noexcept = default;
osc::StoFileSimulation& osc::StoFileSimulation::operator=(StoFileSimulation&&) noexcept = default;
osc::StoFileSimulation::~StoFileSimulation() noexcept = default;

SynchronizedValueGuard<const OpenSim::Model> osc::StoFileSimulation::impl_get_model() const
{
    return impl_->get_model();
}

ptrdiff_t osc::StoFileSimulation::impl_get_num_reports() const
{
    return impl_->get_num_reports();
}

SimulationReport osc::StoFileSimulation::impl_get_simulation_report(ptrdiff_t report_index) const
{
    return impl_->get_simulation_report(report_index);
}

std::vector<SimulationReport> osc::StoFileSimulation::impl_get_all_simulation_reports() const
{
    return impl_->get_all_simulation_reports();
}

SimulationStatus osc::StoFileSimulation::impl_get_status() const
{
    return impl_->get_status();
}

SimulationClocks osc::StoFileSimulation::impl_get_clocks() const
{
    return impl_->get_clocks();
}

const ParamBlock& osc::StoFileSimulation::impl_get_params() const
{
    return impl_->get_params();
}

std::span<const opyn::SharedOutputExtractor> osc::StoFileSimulation::impl_get_output_extractors()
    const
{
    return impl_->get_output_extractors();
}

float osc::StoFileSimulation::impl_get_fixup_scale_factor() const
{
    return impl_->get_fixup_scale_factor();
}

void osc::StoFileSimulation::impl_set_fixup_scale_factor(float v)
{
    impl_->set_fixup_scale_factor(v);
}

std::shared_ptr<Environment> osc::StoFileSimulation::impl_upd_associated_environment()
{
    return impl_->impl_upd_associated_environment();
}
