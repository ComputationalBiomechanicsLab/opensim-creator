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

        int getNumUserOutputExtractors() const { return implGetNumUserOutputExtractors(); }
        const OutputExtractor& getUserOutputExtractor(int i) const { return implGetUserOutputExtractor(i); }
        std::vector<OutputExtractor> getAllUserOutputExtractors() const;
        void addUserOutputExtractor(const OutputExtractor& o) { implAddUserOutputExtractor(o); }
        void removeUserOutputExtractor(int i) { implRemoveUserOutputExtractor(i); }
        bool hasUserOutputExtractor(const OutputExtractor& o) { return implHasUserOutputExtractor(o); }
        bool removeUserOutputExtractor(const OutputExtractor& o) { return implRemoveUserOutputExtractor(o); }
        bool overwriteOrAddNewUserOutputExtractor(const OutputExtractor& old, const OutputExtractor& newer) { return implOverwriteOrAddNewUserOutputExtractor(old, newer); }
        std::optional<std::filesystem::path> tryPromptToSaveOutputsAsCSV(std::span<const OutputExtractor>) const;
        std::optional<std::filesystem::path> tryPromptToSaveOutputsAsCSV(std::initializer_list<OutputExtractor> il) const
        {
            return tryPromptToSaveOutputsAsCSV(std::span<const OutputExtractor>{il});
        }
        std::optional<std::filesystem::path> tryPromptToSaveAllOutputsAsCSV() const;

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

        virtual int implGetNumUserOutputExtractors() const = 0;
        virtual const OutputExtractor& implGetUserOutputExtractor(int) const = 0;
        virtual void implAddUserOutputExtractor(const OutputExtractor&) = 0;
        virtual void implRemoveUserOutputExtractor(int) = 0;
        virtual bool implHasUserOutputExtractor(const OutputExtractor&) const = 0;
        virtual bool implRemoveUserOutputExtractor(const OutputExtractor&) = 0;
        virtual bool implOverwriteOrAddNewUserOutputExtractor(const OutputExtractor&, const OutputExtractor&) = 0;

        virtual SimulationModelStatePair* implTryGetCurrentSimulationState() = 0;
    };
}
