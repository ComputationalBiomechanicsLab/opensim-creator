#pragma once

#include <libopensimcreator/Documents/Simulation/SimulationClock.h>
#include <libopensimcreator/Documents/Simulation/SimulationReport.h>
#include <libopensimcreator/UI/Simulation/SimulationUILoopingState.h>
#include <libopensimcreator/UI/Simulation/SimulationUIPlaybackState.h>

#include <libopynsim/Documents/output_extractors/shared_output_extractor.h>

#include <initializer_list>
#include <optional>
#include <span>

namespace osc { class SharedOutputExtractor; }
namespace osc { class SimulationModelStatePair; }
namespace osc { class ISimulation; }

namespace osc
{
    // virtual API for the simulator UI (e.g. the simulator tab)
    //
    // this is how individual widgets within a simulator UI communicate with the simulator UI
    class ISimulatorUIAPI {
    protected:
        ISimulatorUIAPI() = default;
        ISimulatorUIAPI(const ISimulatorUIAPI&) = default;
        ISimulatorUIAPI(ISimulatorUIAPI&&) noexcept = default;
        ISimulatorUIAPI& operator=(const ISimulatorUIAPI&) = default;
        ISimulatorUIAPI& operator=(ISimulatorUIAPI&&) noexcept = default;
    public:
        virtual ~ISimulatorUIAPI() noexcept = default;

        const ISimulation& getSimulation() const { return implGetSimulation(); }
        ISimulation& updSimulation() { return implUpdSimulation(); }

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

        void tryPromptToSaveOutputsAsCSV(std::span<const SharedOutputExtractor>, bool openInDefaultApp) const;
        void tryPromptToSaveOutputsAsCSV(std::initializer_list<SharedOutputExtractor> il, bool openInDefaultApp) const
        {
            tryPromptToSaveOutputsAsCSV(std::span<const SharedOutputExtractor>{il}, openInDefaultApp);
        }
        void tryPromptToSaveAllOutputsAsCSV(std::span<const SharedOutputExtractor>, bool andOpenInDefaultApp = false) const;

        SimulationModelStatePair* tryGetCurrentSimulationState() { return implTryGetCurrentSimulationState(); }

    private:
        virtual const ISimulation& implGetSimulation() const = 0;
        virtual ISimulation& implUpdSimulation() = 0;

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
