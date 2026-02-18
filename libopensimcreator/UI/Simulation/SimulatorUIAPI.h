#pragma once

#include <libopensimcreator/Documents/Simulation/SimulationClock.h>
#include <libopensimcreator/Documents/Simulation/SimulationReport.h>
#include <libopensimcreator/UI/Simulation/SimulationUILoopingState.h>
#include <libopensimcreator/UI/Simulation/SimulationUIPlaybackState.h>

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>

#include <initializer_list>
#include <optional>
#include <span>

namespace opyn { class SharedOutputExtractor; }
namespace osc { class SimulationModelStatePair; }
namespace osc { class AbstractSimulation; }

namespace osc
{
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

        const AbstractSimulation& getSimulation() const { return implGetSimulation(); }
        AbstractSimulation& updSimulation() { return implUpdSimulation(); }

        SimulationUIPlaybackState getSimulationPlaybackState() { return implGetSimulationPlaybackState(); }
        void setSimulationPlaybackState(SimulationUIPlaybackState s) { implSetSimulationPlaybackState(s); }

        virtual SimulationUILoopingState getSimulationLoopingState() const { return implGetSimulationLoopingState(); }
        virtual void setSimulationLoopingState(SimulationUILoopingState s) { implSetSimulationLoopingState(s); }

        float getSimulationPlaybackSpeed() { return implGetSimulationPlaybackSpeed(); }
        void setSimulationPlaybackSpeed(float v) { implSetSimulationPlaybackSpeed(v); }

        SimulationClock::time_point getSimulationScrubTime() { return implGetSimulationScrubTime(); }

        void stepBack() { implStepBack(); }
        void stepForward() { implStepForward(); }

        void setSimulationScrubTime(SimulationClock::time_point t) { implSetSimulationScrubTime(t); }

        std::optional<SimulationReport> trySelectReportBasedOnScrubbing() { return implTrySelectReportBasedOnScrubbing(); }

        void tryPromptToSaveOutputsAsCSV(std::span<const opyn::SharedOutputExtractor>, bool openInDefaultApp) const;
        void tryPromptToSaveOutputsAsCSV(std::initializer_list<opyn::SharedOutputExtractor> il, bool openInDefaultApp) const
        {
            tryPromptToSaveOutputsAsCSV(std::span<const opyn::SharedOutputExtractor>{il}, openInDefaultApp);
        }
        void tryPromptToSaveAllOutputsAsCSV(std::span<const opyn::SharedOutputExtractor>, bool andOpenInDefaultApp = false) const;

        SimulationModelStatePair* tryGetCurrentSimulationState() { return implTryGetCurrentSimulationState(); }

    private:
        virtual const AbstractSimulation& implGetSimulation() const = 0;
        virtual AbstractSimulation& implUpdSimulation() = 0;

        virtual SimulationUIPlaybackState implGetSimulationPlaybackState() = 0;
        virtual void implSetSimulationPlaybackState(SimulationUIPlaybackState) = 0;
        virtual SimulationUILoopingState implGetSimulationLoopingState() const = 0;
        virtual void implSetSimulationLoopingState(SimulationUILoopingState) = 0;
        virtual float implGetSimulationPlaybackSpeed() = 0;
        virtual void implSetSimulationPlaybackSpeed(float) = 0;
        virtual SimulationClock::time_point implGetSimulationScrubTime() = 0;
        virtual void implSetSimulationScrubTime(SimulationClock::time_point) = 0;
        virtual void implStepBack() = 0;
        virtual void implStepForward() = 0;
        virtual std::optional<SimulationReport> implTrySelectReportBasedOnScrubbing() = 0;

        virtual SimulationModelStatePair* implTryGetCurrentSimulationState() = 0;
    };
}
