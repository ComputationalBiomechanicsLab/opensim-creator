#include "forward_dynamic_simulation.h"

#include <libopensimcreator/documents/model/basic_model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/simulation/forward_dynamic_simulator.h>
#include <libopensimcreator/documents/simulation/forward_dynamic_simulator_params.h>
#include <libopensimcreator/documents/simulation/simulation_clock.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/documents/simulation/simulation_status.h>
#include <libopensimcreator/documents/param_block.h>

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <liboscar/utilities/synchronized_value.h>
#include <liboscar/utilities/synchronized_value_guard.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

using namespace osc;

// helpers
namespace
{
    // creates a simulator that's hooked up to the reports vector
    ForwardDynamicSimulator make_simulation(
        BasicModelStatePairWithSharedEnvironment p,
        const ForwardDynamicSimulatorParams& params,
        SynchronizedValue<std::vector<SimulationReport>>& report_queue)
    {
        auto callback = [&](SimulationReport r) {
            auto reports_guard = report_queue.lock();
            reports_guard->push_back(std::move(r));
        };
        return ForwardDynamicSimulator{std::move(p), params, std::move(callback)};
    }

    std::vector<opyn::SharedOutputExtractor> get_fd_simulator_output_extractors_as_vector()
    {
        std::vector<opyn::SharedOutputExtractor> rv;
        const int n_output_extractors = get_num_fd_simulator_output_extractors();
        rv.reserve(n_output_extractors);
        for (int i = 0; i < n_output_extractors; ++i) {
            rv.push_back(get_fd_simulator_output_extractor(i));
        }
        return rv;
    }
}

class osc::ForwardDynamicSimulation::Impl final {
public:
    Impl(BasicModelStatePairWithSharedEnvironment p, const ForwardDynamicSimulatorParams& params) :
        model_state_{std::move(p)},
        simulation_{make_simulation(*model_state_.lock(), params, report_queue_)},
        params_{params},
        params_as_param_block_{to_param_block(params)},
        simulator_output_extractors_(get_fd_simulator_output_extractors_as_vector())
    {}

    void join() { simulation_.join(); }

    SynchronizedValueGuard<const OpenSim::Model> get_model() const
    {
        return model_state_.lock_child<OpenSim::Model>(
            [](const BasicModelStatePairWithSharedEnvironment& p) -> decltype(auto) {
                return p.get_model();
            });
    }

    ptrdiff_t get_num_reports() const
    {
        pop_reports_hack();
        return reports_.size();
    }

    SimulationReport get_simulation_report(ptrdiff_t report_index) const
    {
        pop_reports_hack();
        return reports_.at(report_index);
    }

    std::vector<SimulationReport> get_all_simulation_reports() const
    {
        pop_reports_hack();
        return reports_;
    }

    SimulationStatus get_status() const { return simulation_.get_status(); }

    SimulationClock::time_point get_start_time() const
    {
        return SimulationClock::start() +
               SimulationClock::duration{model_state_.lock()->get_state().getTime()};
    }

    SimulationClock::time_point get_cur_time() const
    {
        pop_reports_hack();

        if (not reports_.empty()) {
            return SimulationClock::start() +
                   SimulationClock::duration{reports_.back().state().getTime()};
        } else {
            return get_start_time();
        }
    }

    SimulationClocks get_clocks() const
    {
        auto start = get_start_time();
        auto end = params_.final_time;
        return SimulationClocks{{start, end}, get_cur_time()};
    }

    const ParamBlock& get_params() const { return params_as_param_block_; }

    std::span<const opyn::SharedOutputExtractor> get_output_extractors() const
    {
        return simulator_output_extractors_;
    }

    void request_new_end_time(SimulationClock::time_point new_end_time)
    {
        const SimulationClock::time_point old_end_time = get_clocks().end();

        if (new_end_time == old_end_time) {
            return;  // nothing to change
        }

        // before doing anything, sychronously stop current simulation and
        // ensure the report queue is popped
        {
            stop();
            pop_reports_hack();
        }

        // if necessary, truncate any dangling reports
        if (new_end_time < old_end_time and not reports_.empty()) {

            const auto report_before_or_equal_to_new_end_time =
                [new_end_time](const SimulationReport& r) {
                    return r.get_time() <= new_end_time;
                };
            const auto it =
                find_if(reports_.rbegin(), reports_.rend(), report_before_or_equal_to_new_end_time);
            reports_.erase(it.base(), reports_.end());
        }

        // update the simulation parameters to reflect the new end-time
        {
            params_.final_time = new_end_time;
            params_as_param_block_ = to_param_block(params_);
        }

        // edge-case: if the latest available report has an end-time equal to `t`, then
        // our work is complete
        if (not reports_.empty() and reports_.back().get_time() == new_end_time) {
            return;
        }

        // otherwise, create a new simulator with the new parameters
        {
            const auto guard = model_state_.lock();
            const SimTK::State& latest_state =
                reports_.empty() ? guard->get_state() : reports_.back().state();

            simulation_ = make_simulation(
                BasicModelStatePairWithSharedEnvironment{guard->get_model(), latest_state},
                params_,
                report_queue_);

            // note: the report collection method should ensure that double-reports
            // aren't collected
        }
    }

    void request_stop() { simulation_.request_stop(); }

    void stop() { simulation_.stop(); }

    float get_fixup_scale_factor() const { return model_state_.lock()->get_fixup_scale_factor(); }

    void set_fixup_scale_factor(float v) { model_state_.lock()->set_fixup_scale_factor(v); }

    std::shared_ptr<Environment> impl_upd_associated_environment() const
    {
        return model_state_.lock()->try_upd_environment();
    }

private:
    // MUST be done from the UI thread
    //
    // the reason this insane hack is necessary is because the background thread
    // requires access to the UI thread's copy of the model in order to perform
    // the realization step
    void pop_reports_hack() const
    {
        auto& reports = const_cast<std::vector<SimulationReport>&>(reports_);

        // handle double-reporting (e.g. due to `requestNewEndTime`) by checking
        // the time of each incoming reports against the latest already collected
        std::optional<SimulationClock::time_point> latest_report_time;
        if (not reports.empty()) {
            latest_report_time = reports.back().get_time();
        }

        const size_t n_reports_before = reports.size();
        size_t n_added = 0;

        // pop them onto the local reports queue
        {
            auto guard =
                const_cast<SynchronizedValue<std::vector<SimulationReport>>&>(report_queue_).lock();
            reports.reserve(reports.size() + guard->size());
            for (SimulationReport& report : *guard) {
                if (report.get_time() == latest_report_time) {
                    continue;  // filter out duplicate reports (e.g. due to `requestNewEndTime`)
                }

                reports.push_back(std::move(report));
                ++n_added;
            }
            guard->clear();
        }

        if (n_added <= 0) {
            return;
        }

        // ensure all reports are realized on the UI model
        auto model_lock = model_state_.lock();
        for (auto it = reports.begin() + n_reports_before; it != reports.end(); ++it) {
            model_lock->get_model().realizeReport(it->upd_state_hack());
        }
    }

    SynchronizedValue<BasicModelStatePairWithSharedEnvironment> model_state_;
    SynchronizedValue<std::vector<SimulationReport>> report_queue_;
    std::vector<SimulationReport> reports_;
    ForwardDynamicSimulator simulation_;
    ForwardDynamicSimulatorParams params_;
    ParamBlock params_as_param_block_;
    std::vector<opyn::SharedOutputExtractor> simulator_output_extractors_;
};

osc::ForwardDynamicSimulation::ForwardDynamicSimulation(
    BasicModelStatePairWithSharedEnvironment ms, const ForwardDynamicSimulatorParams& params) :

    impl_{std::make_unique<Impl>(std::move(ms), params)}
{}
osc::ForwardDynamicSimulation::ForwardDynamicSimulation(ForwardDynamicSimulation&&) noexcept =
    default;
osc::ForwardDynamicSimulation& osc::ForwardDynamicSimulation::operator=(
    ForwardDynamicSimulation&&) noexcept = default;
osc::ForwardDynamicSimulation::~ForwardDynamicSimulation() noexcept = default;

void osc::ForwardDynamicSimulation::join()
{
    impl_->join();
}

SynchronizedValueGuard<const OpenSim::Model> osc::ForwardDynamicSimulation::impl_get_model() const
{
    return impl_->get_model();
}

ptrdiff_t osc::ForwardDynamicSimulation::impl_get_num_reports() const
{
    return impl_->get_num_reports();
}

SimulationReport osc::ForwardDynamicSimulation::impl_get_simulation_report(
    ptrdiff_t report_index) const
{
    return impl_->get_simulation_report(report_index);
}

std::vector<SimulationReport> osc::ForwardDynamicSimulation::impl_get_all_simulation_reports() const
{
    return impl_->get_all_simulation_reports();
}

SimulationStatus osc::ForwardDynamicSimulation::impl_get_status() const
{
    return impl_->get_status();
}

SimulationClocks osc::ForwardDynamicSimulation::impl_get_clocks() const
{
    return impl_->get_clocks();
}

const ParamBlock& osc::ForwardDynamicSimulation::impl_get_params() const
{
    return impl_->get_params();
}

std::span<const opyn::SharedOutputExtractor> osc::ForwardDynamicSimulation::
    impl_get_output_extractors() const
{
    return impl_->get_output_extractors();
}

void osc::ForwardDynamicSimulation::impl_request_new_end_time(SimulationClock::time_point t)
{
    impl_->request_new_end_time(t);
}

void osc::ForwardDynamicSimulation::impl_request_stop()
{
    impl_->request_stop();
}

void osc::ForwardDynamicSimulation::impl_stop()
{
    impl_->stop();
}

float osc::ForwardDynamicSimulation::impl_get_fixup_scale_factor() const
{
    return impl_->get_fixup_scale_factor();
}

void osc::ForwardDynamicSimulation::impl_set_fixup_scale_factor(float v)
{
    impl_->set_fixup_scale_factor(v);
}

std::shared_ptr<Environment> osc::ForwardDynamicSimulation::impl_upd_associated_environment()
{
    return impl_->impl_upd_associated_environment();
}
