#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/UI/Simulation/SimulationUIPlaybackState.h>
#include <OpenSimCreator/UI/Simulation/SimulationUILoopingState.h>

#include <filesystem>
#include <initializer_list>
#include <optional>
#include <span>
#include <vector>

namespace osc { class OutputExtractor; }
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
        ISimulatorUIAPI(ISimulatorUIAPI const&) = default;
        ISimulatorUIAPI(ISimulatorUIAPI&&) noexcept = default;
        ISimulatorUIAPI& operator=(ISimulatorUIAPI const&) = default;
        ISimulatorUIAPI& operator=(ISimulatorUIAPI&&) noexcept = default;
    public:
        virtual ~ISimulatorUIAPI() noexcept = default;

        ISimulation const& getSimulation() const { return implGetSimulation(); }
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

        int getNumUserOutputExtractors() const { return implGetNumUserOutputExtractors(); }
        OutputExtractor const& getUserOutputExtractor(int i) const { return implGetUserOutputExtractor(i); }
        std::vector<OutputExtractor> getAllUserOutputExtractors() const;
        void addUserOutputExtractor(OutputExtractor const& o) { implAddUserOutputExtractor(o); }
        void removeUserOutputExtractor(int i) { implRemoveUserOutputExtractor(i); }
        bool hasUserOutputExtractor(OutputExtractor const& o) { return implHasUserOutputExtractor(o); }
        bool removeUserOutputExtractor(OutputExtractor const& o) { return implRemoveUserOutputExtractor(o); }
        bool overwriteOrAddNewUserOutputExtractor(OutputExtractor const& old, OutputExtractor const& newer) { return implOverwriteOrAddNewUserOutputExtractor(old, newer); }
        std::optional<std::filesystem::path> tryPromptToSaveOutputsAsCSV(std::span<OutputExtractor const>) const;
        std::optional<std::filesystem::path> tryPromptToSaveOutputsAsCSV(std::initializer_list<OutputExtractor> il) const
        {
            return tryPromptToSaveOutputsAsCSV(std::span<OutputExtractor const>{il});
        }
        std::optional<std::filesystem::path> tryPromptToSaveAllOutputsAsCSV() const;

        SimulationModelStatePair* tryGetCurrentSimulationState() { return implTryGetCurrentSimulationState(); }

    private:
        virtual ISimulation const& implGetSimulation() const = 0;
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

        virtual int implGetNumUserOutputExtractors() const = 0;
        virtual OutputExtractor const& implGetUserOutputExtractor(int) const = 0;
        virtual void implAddUserOutputExtractor(OutputExtractor const&) = 0;
        virtual void implRemoveUserOutputExtractor(int) = 0;
        virtual bool implHasUserOutputExtractor(OutputExtractor const&) const = 0;
        virtual bool implRemoveUserOutputExtractor(OutputExtractor const&) = 0;
        virtual bool implOverwriteOrAddNewUserOutputExtractor(OutputExtractor const&, OutputExtractor const&) = 0;

        virtual SimulationModelStatePair* implTryGetCurrentSimulationState() = 0;
    };
}
