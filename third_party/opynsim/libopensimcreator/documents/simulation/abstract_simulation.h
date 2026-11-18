#pragma once

#include <libopensimcreator/documents/simulation/simulation_clock.h>
#include <libopensimcreator/documents/simulation/simulation_clocks.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/documents/simulation/simulation_status.h>

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
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
    class ParamBlock;

    // a virtual simulation could be backed by (e.g.):
    //
    // - a real "live" forward-dynamic simulation
    // - an .sto file
    //
    // the GUI code shouldn't care about the specifics - it's up to each concrete
    // implementation to ensure this API is obeyed w.r.t. multithreading etc.
    class AbstractSimulation {
    protected:
        AbstractSimulation() = default;
        AbstractSimulation(const AbstractSimulation&) = default;
        AbstractSimulation(AbstractSimulation&&) noexcept = default;
        AbstractSimulation& operator=(const AbstractSimulation&) = default;
        AbstractSimulation& operator=(AbstractSimulation&&) noexcept = default;

    public:
        virtual ~AbstractSimulation() noexcept = default;

        // the reason why the model is mutex-guarded is because OpenSim has a bunch of
        // `const-` interfaces that are only "logically const" in a single-threaded
        // environment.
        //
        // this can lead to mayhem if (e.g.) the model is actually being mutated by
        // multiple threads concurrently
        SynchronizedValueGuard<const OpenSim::Model> get_model() const { return impl_get_model(); }

        size_t get_num_reports() const { return impl_get_num_reports(); }

        SimulationReport get_simulation_report(ptrdiff_t report_index) const
        {
            return impl_get_simulation_report(report_index);
        }

        std::vector<SimulationReport> get_all_simulation_reports() const
        {
            return impl_get_all_simulation_reports();
        }

        SimulationStatus get_status() const { return impl_get_status(); }

        SimulationClock::time_point get_cur_time() const { return impl_get_clocks().current(); }

        SimulationClock::time_point get_start_time() const { return impl_get_clocks().start(); }

        SimulationClock::time_point get_end_time() const { return impl_get_clocks().end(); }

        SimulationClock::duration get_duration() const
        {
            const auto clocks = impl_get_clocks();
            return clocks.end() - clocks.start();
        }

        bool can_change_end_time() const { return impl_can_change_end_time(); }

        void request_new_end_time(SimulationClock::time_point t) { impl_request_new_end_time(t); }

        float get_progress() const { return impl_get_clocks().progress(); }

        const ParamBlock& get_params() const { return impl_get_params(); }

        std::span<const opyn::SharedOutputExtractor> get_output_extractors() const
        {
            return impl_get_output_extractors();
        }

        void request_stop() { impl_request_stop(); }

        void stop() { impl_stop(); }

        float get_fixup_scale_factor() const { return impl_get_fixup_scale_factor(); }

        void set_fixup_scale_factor(float new_scale_factor)
        {
            impl_set_fixup_scale_factor(new_scale_factor);
        }

        std::shared_ptr<Environment> try_upd_environment()
        {
            return impl_upd_associated_environment();
        }

    private:
        // TODO: this shouldn't be mutex-xed: it can cause runtime issues
        virtual SynchronizedValueGuard<const OpenSim::Model> impl_get_model() const = 0;

        virtual ptrdiff_t impl_get_num_reports() const = 0;
        virtual SimulationReport impl_get_simulation_report(ptrdiff_t) const = 0;
        virtual std::vector<SimulationReport> impl_get_all_simulation_reports() const = 0;

        virtual SimulationStatus impl_get_status() const = 0;
        virtual SimulationClocks impl_get_clocks() const = 0;
        virtual const ParamBlock& impl_get_params() const = 0;
        virtual std::span<const opyn::SharedOutputExtractor> impl_get_output_extractors() const = 0;

        virtual bool impl_can_change_end_time() const { return false; }
        virtual void impl_request_new_end_time(SimulationClock::time_point) {}

        virtual void impl_request_stop() {}  // only applicable for "live" simulations
        virtual void impl_stop() {}          // only applicable for "live" simulations

        virtual float impl_get_fixup_scale_factor() const = 0;
        virtual void impl_set_fixup_scale_factor(float) = 0;

        virtual std::shared_ptr<Environment> impl_upd_associated_environment() = 0;
    };
}
