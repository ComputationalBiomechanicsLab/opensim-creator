#pragma once

#include <libopensimcreator/documents/simulation/simulation_clock.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/ui/simulation/simulation_ui_looping_state.h>
#include <libopensimcreator/ui/simulation/simulation_ui_playback_state.h>

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>

#include <initializer_list>
#include <optional>
#include <span>

namespace opyn
{
    class SharedOutputExtractor;
}

namespace osc
{
    class AbstractSimulation;
    class SimulationModelStatePair;

    // virtual API for the simulator UI (e.g. the simulator tab)
    //
    // this is how individual widgets within a simulator UI communicate with the simulator UI
    class SimulatorUIAPI {
    protected:
        SimulatorUIAPI() = default;
        SimulatorUIAPI(const SimulatorUIAPI&) = default;
        SimulatorUIAPI(SimulatorUIAPI&&) noexcept = default;
        SimulatorUIAPI& operator=(const SimulatorUIAPI&) = default;
        SimulatorUIAPI& operator=(SimulatorUIAPI&&) noexcept = default;

    public:
        virtual ~SimulatorUIAPI() noexcept = default;

        const AbstractSimulation& get_simulation() const { return impl_get_simulation(); }
        AbstractSimulation& upd_simulation() { return impl_upd_simulation(); }

        SimulationUIPlaybackState get_simulation_playback_state()
        {
            return impl_get_simulation_playback_state();
        }
        void set_simulation_playback_state(SimulationUIPlaybackState s)
        {
            impl_set_simulation_playback_state(s);
        }

        virtual SimulationUILoopingState get_simulation_looping_state() const
        {
            return impl_get_simulation_looping_state();
        }
        virtual void set_simulation_looping_state(SimulationUILoopingState s)
        {
            impl_set_simulation_looping_state(s);
        }

        float get_simulation_playback_speed() { return impl_get_simulation_playback_speed(); }
        void set_simulation_playback_speed(float v) { impl_set_simulation_playback_speed(v); }

        SimulationClock::time_point get_simulation_scrub_time()
        {
            return impl_get_simulation_scrub_time();
        }

        void step_back() { impl_step_back(); }
        void step_forward() { impl_step_forward(); }

        void set_simulation_scrub_time(SimulationClock::time_point t)
        {
            impl_set_simulation_scrub_time(t);
        }

        std::optional<SimulationReport> try_select_report_based_on_scrubbing()
        {
            return impl_try_select_report_based_on_scrubbing();
        }

        void try_prompt_to_save_outputs_as_csv(
            std::span<const opyn::SharedOutputExtractor>, bool open_in_default_app) const;
        void try_prompt_to_save_outputs_as_csv(
            std::initializer_list<opyn::SharedOutputExtractor> il, bool open_in_default_app) const
        {
            try_prompt_to_save_outputs_as_csv(
                std::span<const opyn::SharedOutputExtractor>{il}, open_in_default_app);
        }
        void try_prompt_to_save_all_outputs_as_csv(
            std::span<const opyn::SharedOutputExtractor>,
            bool and_open_in_default_app = false) const;

        SimulationModelStatePair* try_get_current_simulation_state()
        {
            return impl_try_get_current_simulation_state();
        }

    private:
        virtual const AbstractSimulation& impl_get_simulation() const = 0;
        virtual AbstractSimulation& impl_upd_simulation() = 0;

        virtual SimulationUIPlaybackState impl_get_simulation_playback_state() = 0;
        virtual void impl_set_simulation_playback_state(SimulationUIPlaybackState) = 0;
        virtual SimulationUILoopingState impl_get_simulation_looping_state() const = 0;
        virtual void impl_set_simulation_looping_state(SimulationUILoopingState) = 0;
        virtual float impl_get_simulation_playback_speed() = 0;
        virtual void impl_set_simulation_playback_speed(float) = 0;
        virtual SimulationClock::time_point impl_get_simulation_scrub_time() = 0;
        virtual void impl_set_simulation_scrub_time(SimulationClock::time_point) = 0;
        virtual void impl_step_back() = 0;
        virtual void impl_step_forward() = 0;
        virtual std::optional<SimulationReport> impl_try_select_report_based_on_scrubbing() = 0;

        virtual SimulationModelStatePair* impl_try_get_current_simulation_state() = 0;
    };
}
