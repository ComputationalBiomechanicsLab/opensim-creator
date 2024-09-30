#pragma once

#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClocks.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/Documents/Simulation/SimulationStatus.h>

#include <oscar/Utils/SynchronizedValueGuard.h>

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace osc { class Environment; }
namespace osc { class OutputExtractor; }
namespace osc { class ParamBlock; }
namespace OpenSim { class Model; }

namespace osc
{
    // a virtual simulation could be backed by (e.g.):
    //
    // - a real "live" forward-dynamic simulation
    // - an .sto file
    //
    // the GUI code shouldn't care about the specifics - it's up to each concrete
    // implementation to ensure this API is obeyed w.r.t. multithreading etc.
    class ISimulation {
    protected:
        ISimulation() = default;
        ISimulation(const ISimulation&) = default;
        ISimulation(ISimulation&&) noexcept = default;
        ISimulation& operator=(const ISimulation&) = default;
        ISimulation& operator=(ISimulation&&) noexcept = default;
    public:
        virtual ~ISimulation() noexcept = default;

        // the reason why the model is mutex-guarded is because OpenSim has a bunch of
        // `const-` interfaces that are only "logically const" in a single-threaded
        // environment.
        //
        // this can lead to mayhem if (e.g.) the model is actually being mutated by
        // multiple threads concurrently
        SynchronizedValueGuard<const OpenSim::Model> getModel() const
        {
            return implGetModel();
        }

        size_t getNumReports() const
        {
            return implGetNumReports();
        }

        SimulationReport getSimulationReport(ptrdiff_t reportIndex) const
        {
            return implGetSimulationReport(reportIndex);
        }

        std::vector<SimulationReport> getAllSimulationReports() const
        {
            return implGetAllSimulationReports();
        }

        SimulationStatus getStatus() const
        {
            return implGetStatus();
        }

        SimulationClock::time_point getCurTime() const
        {
            return implGetClocks().current();
        }

        SimulationClock::time_point getStartTime() const
        {
            return implGetClocks().start();
        }

        SimulationClock::time_point getEndTime() const
        {
            return implGetClocks().end();
        }

        bool canChangeEndTime() const
        {
            return implCanChangeEndTime();
        }

        void requestNewEndTime(SimulationClock::time_point t)
        {
            implRequestNewEndTime(t);
        }

        float getProgress() const
        {
            return implGetClocks().progress();
        }

        const ParamBlock& getParams() const
        {
            return implGetParams();
        }

        std::span<const OutputExtractor> getOutputExtractors() const
        {
            return implGetOutputExtractors();
        }

        void requestStop()
        {
            implRequestStop();
        }

        void stop()
        {
            implStop();
        }

        float getFixupScaleFactor() const
        {
            return implGetFixupScaleFactor();
        }

        void setFixupScaleFactor(float newScaleFactor)
        {
            implSetFixupScaleFactor(newScaleFactor);
        }

        std::shared_ptr<Environment> tryUpdEnvironment() { return implUpdAssociatedEnvironment(); }
    private:
        virtual SynchronizedValueGuard<const OpenSim::Model> implGetModel() const = 0;

        virtual ptrdiff_t implGetNumReports() const = 0;
        virtual SimulationReport implGetSimulationReport(ptrdiff_t) const = 0;
        virtual std::vector<SimulationReport> implGetAllSimulationReports() const = 0;

        virtual SimulationStatus implGetStatus() const = 0;
        virtual SimulationClocks implGetClocks() const = 0;
        virtual const ParamBlock& implGetParams() const = 0;
        virtual std::span<const OutputExtractor> implGetOutputExtractors() const = 0;

        virtual bool implCanChangeEndTime() const { return false; }
        virtual void implRequestNewEndTime(SimulationClock::time_point) {}

        virtual void implRequestStop() {}  // only applicable for "live" simulations
        virtual void implStop() {}  // only applicable for "live" simulations

        virtual float implGetFixupScaleFactor() const = 0;
        virtual void implSetFixupScaleFactor(float) = 0;

        virtual std::shared_ptr<Environment> implUpdAssociatedEnvironment() = 0;
    };
}
