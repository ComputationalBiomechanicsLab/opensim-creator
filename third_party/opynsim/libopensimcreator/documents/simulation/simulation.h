#pragma once

#include <libopensimcreator/documents/simulation/abstract_simulation.h>
#include <libopensimcreator/documents/simulation/simulation_clock.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/documents/simulation/simulation_status.h>

#include <liboscar/utilities/synchronized_value_guard.h>

#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace opyn
{
    class SharedOutputExtractor;
}

namespace OpenSim
{
    class Model;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    class Environment;
    class ParamBlock;

    // a concrete value type wrapper for an `AbstractSimulation`
    //
    // This is a value-type that can be compared, hashed, etc. for easier usage
    // by other parts of osc (e.g. aggregators, plotters)
    class Simulation final {
    public:
        template<std::derived_from<AbstractSimulation> TConcreteSimulation>
        explicit Simulation(TConcreteSimulation&& simulation) :
            simulation_{std::make_unique<TConcreteSimulation>(
                std::forward<TConcreteSimulation>(simulation))}
        {}

        SynchronizedValueGuard<const OpenSim::Model> get_model() const
        {
            return simulation_->get_model();
        }

        size_t get_num_reports() const { return simulation_->get_num_reports(); }
        SimulationReport get_simulation_report(ptrdiff_t report_index) const
        {
            return simulation_->get_simulation_report(report_index);
        }
        std::vector<SimulationReport> get_all_simulation_reports() const
        {
            return simulation_->get_all_simulation_reports();
        }

        SimulationStatus get_status() const { return simulation_->get_status(); }
        SimulationClock::time_point get_cur_time() { return simulation_->get_cur_time(); }
        SimulationClock::time_point get_start_time() const { return simulation_->get_start_time(); }
        SimulationClock::time_point get_end_time() const { return simulation_->get_end_time(); }
        SimulationClock::duration get_duration() const { return simulation_->get_duration(); }
        float get_progress() const { return simulation_->get_progress(); }
        const ParamBlock& get_params() const { return simulation_->get_params(); }
        std::span<const opyn::SharedOutputExtractor> get_outputs() const
        {
            return simulation_->get_output_extractors();
        }

        bool can_change_end_time() const { return simulation_->can_change_end_time(); }
        void request_new_end_time(SimulationClock::time_point t)
        {
            simulation_->request_new_end_time(t);
        }

        void request_stop() { simulation_->request_stop(); }
        void stop() { simulation_->stop(); }

        float get_fixup_scale_factor() const { return simulation_->get_fixup_scale_factor(); }
        void set_fixup_scale_factor(float v) { simulation_->set_fixup_scale_factor(v); }

        std::shared_ptr<Environment> try_upd_environment()
        {
            return simulation_->try_upd_environment();
        }

        operator AbstractSimulation&() { return *simulation_; }
        operator const AbstractSimulation&() const { return *simulation_; }

    private:
        std::unique_ptr<AbstractSimulation> simulation_;
    };
}
