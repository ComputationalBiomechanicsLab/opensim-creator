#include "forward_dynamic_simulator.h"

#include <libopensimcreator/documents/model/basic_model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/simulation/forward_dynamic_simulator_params.h>
#include <libopensimcreator/documents/simulation/simulation_clock.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/documents/simulation/simulation_status.h>

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/integrator_output_extractor.h>
#include <libopynsim/documents/output_extractors/multi_body_system_output_extractor.h>
#include <libopynsim/solvers/integrator_method.h>
#include <liboscar/platform/log.h>
#include <liboscar/shims/cpp20/stop_token.h>
#include <liboscar/shims/cpp20/thread.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/hash_helpers.h>
#include <liboscar/utilities/perf_clock.h>
#include <liboscar/utilities/uid.h>
#include <OpenSim/Common/Exception.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <simmath/Integrator.h>
#include <simmath/TimeStepper.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace OpenSim
{
    class Component;
}

namespace cpp20 = osc::cpp20;
using namespace osc;

namespace
{
    UID get_walltime_uid()
    {
        static const UID s_walltime_uid;
        return s_walltime_uid;
    }

    UID get_step_duration_uid()
    {
        static const UID s_step_duration_uid;
        return s_step_duration_uid;
    }

    // exclusively owned input data
    class SimulatorThreadInput final {
    public:
        SimulatorThreadInput(
            BasicModelStatePairWithSharedEnvironment model_state,
            const ForwardDynamicSimulatorParams& params,
            std::function<void(SimulationReport)> on_report_from_bg_thread) :

            model_state_{std::move(model_state)},
            params_{params},
            report_callback_{std::move(on_report_from_bg_thread)}
        {}

        const SimTK::MultibodySystem& get_multi_body_system() const
        {
            return model_state_.get_model().getMultibodySystem();
        }
        const SimTK::State& get_state() const { return model_state_.get_state(); }
        const ForwardDynamicSimulatorParams& get_params() const { return params_; }
        void emit_report(SimulationReport report) { report_callback_(std::move(report)); }

    private:
        BasicModelStatePairWithSharedEnvironment model_state_;
        ForwardDynamicSimulatorParams params_;
        std::function<void(SimulationReport)> report_callback_;
    };

    // data that's shared with the UI thread
    class SharedState final {
    public:
        SimulationStatus get_status() const
        {
            return static_cast<SimulationStatus>(status_.load());
        }

        void set_status(SimulationStatus s) { status_ = static_cast<int>(s); }

    private:
        std::atomic<int> status_ = static_cast<int>(SimulationStatus::Initializing);
    };

    class AuxiliaryVariableOutputExtractor final : public opyn::OutputExtractor {
    public:
        AuxiliaryVariableOutputExtractor(std::string name, std::string description, UID uid) :
            name_{std::move(name)},
            description_{std::move(description)},
            uid_{uid}
        {}

    private:
        CStringView impl_name() const override { return name_; }

        CStringView impl_description() const override { return description_; }

        opyn::OutputExtractorDataType impl_output_type() const override
        {
            return opyn::OutputExtractorDataType::Float;
        }

        opyn::OutputValueExtractor impl_output_value_extractor(
            const OpenSim::Component&) const override
        {
            return opyn::OutputValueExtractor{
                [id = uid_](const opyn::StateViewWithMetadata& state) {
                    return Variant{state.get_auxiliary_value(id).value_or(-1337.0f)};
                }};
        }

        std::size_t impl_hash() const override { return hash_of(name_, description_, uid_); }

        bool impl_equals(const OutputExtractor& other) const override
        {
            if (&other == this) {
                return true;
            }

            const auto* const other_t =
                dynamic_cast<const AuxiliaryVariableOutputExtractor*>(&other);
            if (not other_t) {
                return false;
            }

            return name_ == other_t->name_ && description_ == other_t->description_ &&
                   uid_ == other_t->uid_;
        }

        std::string name_;
        std::string description_;
        UID uid_;
    };

    std::vector<opyn::SharedOutputExtractor> create_simulator_output_extractors()
    {
        std::vector<opyn::SharedOutputExtractor> rv;
        rv.reserve(
            2uz + opyn::num_integrator_output_extractors() +
            opyn::num_multi_body_system_output_extractors());

        {
            const opyn::SharedOutputExtractor out =
                opyn::make_output_extractor<AuxiliaryVariableOutputExtractor>(
                    "Wall time",
                    "Total cumulative time spent computing the simulation",
                    get_walltime_uid());
            rv.push_back(out);

            const opyn::SharedOutputExtractor out2 =
                opyn::make_output_extractor<AuxiliaryVariableOutputExtractor>(
                    "Step Wall Time",
                    "How long it took, in wall time, to compute the last integration step",
                    get_step_duration_uid());
            rv.push_back(out2);
        }

        for (int i = 0, len = opyn::num_integrator_output_extractors(); i < len; ++i) {
            rv.push_back(opyn::integrator_output_extractor_dynamic(i));
        }

        for (int i = 0, len = opyn::num_multi_body_system_output_extractors(); i < len; ++i) {
            rv.push_back(opyn::multi_body_system_output_extractor_dynamic(i));
        }

        return rv;
    }

    std::vector<opyn::SharedOutputExtractor> const& get_simulator_output_extractors()
    {
        static const std::vector<opyn::SharedOutputExtractor> s_outputs =
            create_simulator_output_extractors();
        return s_outputs;
    }

    std::unique_ptr<SimTK::Integrator> construct_integrator_with_method(
        const opyn::IntegratorMethod& method, const SimTK::MultibodySystem& system)
    {
        static_assert(osc::num_options_v<opyn::IntegratorMethod> == 7);

        // clang-format off
        switch (method) {
        case opyn::IntegratorMethod::RungeKuttaMerson:   return std::make_unique<SimTK::RungeKuttaMersonIntegrator>(system);
        case opyn::IntegratorMethod::ExplicitEuler:      return std::make_unique<SimTK::ExplicitEulerIntegrator>(system);
        case opyn::IntegratorMethod::RungeKutta2:        return std::make_unique<SimTK::RungeKutta2Integrator>(system);
        case opyn::IntegratorMethod::RungeKutta3:        return std::make_unique<SimTK::RungeKutta3Integrator>(system);
        case opyn::IntegratorMethod::RungeKuttaFeldberg: return std::make_unique<SimTK::RungeKuttaFeldbergIntegrator>(system);
        case opyn::IntegratorMethod::SemiExplicitEuler2: return std::make_unique<SimTK::SemiExplicitEuler2Integrator>(system);
        case opyn::IntegratorMethod::Verlet:             return std::make_unique<SimTK::VerletIntegrator>(system);
        default:                                         std::unreachable();
        }
        // clang-format on
    }

    std::unique_ptr<SimTK::Integrator> create_initialized_integrator(
        const SimulatorThreadInput& input)
    {
        const ForwardDynamicSimulatorParams& params = input.get_params();

        // create + init an integrator
        auto integ = construct_integrator_with_method(
            input.get_params().integrator_method_used, input.get_multi_body_system());
        integ->setInternalStepLimit(params.integrator_step_limit);
        integ->setMinimumStepSize(params.integrator_minimum_step_size.count());
        integ->setMaximumStepSize(params.integrator_maximum_step_size.count());
        integ->setAccuracy(params.integrator_accuracy);
        integ->setFinalTime(params.final_time.time_since_epoch().count());
        integ->setReturnEveryInternalStep(true);  // so that cancellations/interrupts work
        integ->initialize(input.get_state());
        return integ;
    }

    SimulationClock::time_point get_simulation_time(const SimTK::Integrator& integ)
    {
        return SimulationClock::time_point(SimulationClock::duration(integ.getTime()));
    }

    SimulationReport create_simulation_report(
        std::chrono::duration<float> wall_time,
        std::chrono::duration<float> step_duration,
        const SimTK::MultibodySystem& sys,
        const SimTK::Integrator& integrator)
    {
        SimTK::State st = integrator.getState();
        std::unordered_map<UID, float> aux_values;

        // HACK: state needs to be realized on the simulator thread
        st.invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);

        // populate forward dynamic simulator outputs
        {
            aux_values.emplace(get_walltime_uid(), wall_time.count());
            aux_values.emplace(get_step_duration_uid(), step_duration.count());
        }

        // populate integrator outputs
        {
            const int num_outputs = opyn::num_integrator_output_extractors();
            aux_values.reserve(aux_values.size() + num_outputs);
            for (int i = 0; i < num_outputs; ++i) {
                const opyn::IntegratorOutputExtractor& o = opyn::integrator_output_extractor(i);
                aux_values.emplace(o.auxiliary_data_id(), o.extractor_function()(integrator));
            }
        }

        // populate mbs outputs
        {
            const int num_outputs = opyn::num_multi_body_system_output_extractors();
            aux_values.reserve(aux_values.size() + num_outputs);
            for (int i = 0; i < num_outputs; ++i) {
                const opyn::MultiBodySystemOutputExtractor& o =
                    opyn::multi_body_system_output_extractor(i);
                aux_values.emplace(o.auxiliary_data_id(), o.extractor_function()(sys));
            }
        }

        return SimulationReport{std::move(st), std::move(aux_values)};
    }

    // this is the main function that the simulator thread works through (unguarded against
    // exceptions)
    SimulationStatus fd_simulation_main_unguarded(
        cpp20::stop_token stop_token, SimulatorThreadInput& input, SharedState& shared)
    {
        const auto t_sim_start = PerfClock::now();

        const ForwardDynamicSimulatorParams& params = input.get_params();

        // create + init an integrator
        std::unique_ptr<SimTK::Integrator> integ = create_initialized_integrator(input);

        // create + init a timestepper for the integrator
        SimTK::TimeStepper ts{input.get_multi_body_system(), *integ};
        ts.initialize(integ->getState());
        ts.setReportAllSignificantStates(true);  // so that cancellations/interrupts work

        // inform observers that everything has been initialized and the sim is now running
        shared.set_status(SimulationStatus::Running);

        // immediately report t = start
        {
            const std::chrono::duration<float> wall_dur = PerfClock::now() - t_sim_start;
            input.emit_report(
                create_simulation_report(wall_dur, {}, input.get_multi_body_system(), *integ));
        }

        // integrate (t0..tfinal]
        const SimulationClock::time_point t_start = get_simulation_time(*integ);
        SimulationClock::time_point t_last_report = t_start;
        int step = 1;
        while (not integ->isSimulationOver()) {
            // check for cancellation requests
            if (stop_token.stop_requested()) {
                return SimulationStatus::Cancelled;
            }

            // calculate next reporting time
            const SimulationClock::time_point t_next = t_start + (step * params.reporting_interval);

            // perform an integration step
            const auto t_step_start = PerfClock::now();
            const SimTK::Integrator::SuccessfulStepStatus timestep_rv =
                ts.stepTo(t_next.time_since_epoch().count());
            const auto t_step_end = PerfClock::now();

            // handle integrator response
            if (integ->isSimulationOver() &&
                integ->getTerminationReason() != SimTK::Integrator::ReachedFinalTime) {

                // simulation ended because of an error: report the error and exit
                log_error(integ->getTerminationReasonString(integ->getTerminationReason()));
                return SimulationStatus::Error;
            } else if (timestep_rv == SimTK::Integrator::ReachedReportTime) {
                // report the step and continue
                const std::chrono::duration<float> wall_dur = t_step_end - t_sim_start;
                const std::chrono::duration<float> step_dur = t_step_end - t_step_start;
                input.emit_report(create_simulation_report(
                    wall_dur, step_dur, input.get_multi_body_system(), *integ));
                t_last_report = get_simulation_time(*integ);
                ++step;
                continue;
            } else if (timestep_rv == SimTK::Integrator::EndOfSimulation) {
                // if the simulation endpoint is sufficiently ahead of the last report time
                // (1 % of step size), then *also* report the simulation end time. Otherwise,
                // assume that there's an adjacent-enough report
                const SimulationClock::time_point t = get_simulation_time(*integ);
                if ((t_last_report + 0.01 * params.reporting_interval) < t) {
                    const std::chrono::duration<float> wall_dur = t_step_end - t_sim_start;
                    const std::chrono::duration<float> step_dur = t_step_end - t_step_start;
                    input.emit_report(create_simulation_report(
                        wall_dur, step_dur, input.get_multi_body_system(), *integ));
                    t_last_report = t;
                }
                break;
            } else {
                // loop back and do the next timestep
            }
        }

        return SimulationStatus::Completed;
    }

    // MAIN function for the simulator thread
    //
    // guarded against exceptions (which are handled as simulation failures)
    int fd_simulation_main(
        cpp20::stop_token stop_token,
        std::unique_ptr<SimulatorThreadInput> input,
        std::shared_ptr<SharedState> shared)
    {
        SimulationStatus status = SimulationStatus::Error;

        try {
            status = fd_simulation_main_unguarded(std::move(stop_token), *input, *shared);
        } catch (const OpenSim::Exception& ex) {
            log_error("OpenSim::Exception occurred when running a simulation: {}", ex.what());
        } catch (const std::exception& ex) {
            log_error("std::exception occurred when running a simulation: {}", ex.what());
        } catch (...) {
            log_error(
                "an exception with unknown type occurred when running a simulation (no error message available)");
        }

        shared->set_status(status);

        return 0;
    }
}

class osc::ForwardDynamicSimulator::Impl final {
public:
    Impl(
        BasicModelStatePairWithSharedEnvironment model_state,
        const ForwardDynamicSimulatorParams& params,
        std::function<void(SimulationReport)> on_report_from_bg_thread) :

        simulation_params_{params},
        shared_{std::make_shared<SharedState>()},
        simulator_thread_{
            fd_simulation_main,
            std::make_unique<SimulatorThreadInput>(
                std::move(model_state), params, std::move(on_report_from_bg_thread)),
            shared_}
    {}

    SimulationStatus get_status() const { return shared_->get_status(); }

    void join()
    {
        if (simulator_thread_.joinable()) {
            simulator_thread_.join();
        }
    }

    void request_stop() { simulator_thread_.request_stop(); }

    void stop()
    {
        simulator_thread_.request_stop();
        if (simulator_thread_.joinable()) {
            simulator_thread_.join();
        }
    }

    const ForwardDynamicSimulatorParams& params() const { return simulation_params_; }

private:
    ForwardDynamicSimulatorParams simulation_params_;
    std::shared_ptr<SharedState> shared_;
    cpp20::jthread simulator_thread_;
};

int osc::get_num_fd_simulator_output_extractors()
{
    return static_cast<int>(get_simulator_output_extractors().size());
}

opyn::SharedOutputExtractor osc::get_fd_simulator_output_extractor(int idx)
{
    return get_simulator_output_extractors().at(static_cast<size_t>(idx));
}

osc::ForwardDynamicSimulator::ForwardDynamicSimulator(
    BasicModelStatePairWithSharedEnvironment msp,
    const ForwardDynamicSimulatorParams& params,
    std::function<void(SimulationReport)> on_report_from_bg_thread) :

    impl_{std::make_unique<Impl>(std::move(msp), params, std::move(on_report_from_bg_thread))}
{}
osc::ForwardDynamicSimulator::ForwardDynamicSimulator(ForwardDynamicSimulator&&) noexcept = default;
osc::ForwardDynamicSimulator& osc::ForwardDynamicSimulator::operator=(
    ForwardDynamicSimulator&&) noexcept = default;
osc::ForwardDynamicSimulator::~ForwardDynamicSimulator() noexcept = default;

SimulationStatus osc::ForwardDynamicSimulator::get_status() const
{
    return impl_->get_status();
}

void osc::ForwardDynamicSimulator::join()
{
    impl_->join();
}

void osc::ForwardDynamicSimulator::request_stop()
{
    impl_->request_stop();
}

void osc::ForwardDynamicSimulator::stop()
{
    impl_->stop();
}

const ForwardDynamicSimulatorParams& osc::ForwardDynamicSimulator::params() const
{
    return impl_->params();
}
