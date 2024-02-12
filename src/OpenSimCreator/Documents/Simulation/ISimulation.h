#pragma once

#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/Documents/Simulation/SimulationStatus.h>

#include <oscar/Utils/SynchronizedValueGuard.h>

#include <cstddef>
#include <span>
#include <vector>

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
        ISimulation(ISimulation const&) = default;
        ISimulation(ISimulation&&) noexcept = default;
        ISimulation& operator=(ISimulation const&) = default;
        ISimulation& operator=(ISimulation&&) noexcept = default;
    public:
        virtual ~ISimulation() noexcept = default;

        // the reason why the model is mutex-guarded is because OpenSim has a bunch of
        // `const-` interfaces that are only "logically const" in a single-threaded
        // environment.
        //
        // this can lead to mayhem if (e.g.) the model is actually being mutated by
        // multiple threads concurrently
        SynchronizedValueGuard<OpenSim::Model const> getModel() const
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
            return implGetCurTime();
        }

        SimulationClock::time_point getStartTime() const
        {
            return implGetStartTime();
        }

        SimulationClock::time_point getEndTime() const
        {
            return implGetEndTime();
        }

        float getProgress() const
        {
            return implGetProgress();
        }

        ParamBlock const& getParams() const
        {
            return implGetParams();
        }

        std::span<OutputExtractor const> getOutputExtractors() const
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

    private:
        virtual SynchronizedValueGuard<OpenSim::Model const> implGetModel() const = 0;

        virtual ptrdiff_t implGetNumReports() const = 0;
        virtual SimulationReport implGetSimulationReport(ptrdiff_t) const = 0;
        virtual std::vector<SimulationReport> implGetAllSimulationReports() const = 0;

        virtual SimulationStatus implGetStatus() const = 0;
        virtual SimulationClock::time_point implGetCurTime() const = 0;
        virtual SimulationClock::time_point implGetStartTime() const = 0;
        virtual SimulationClock::time_point implGetEndTime() const = 0;
        virtual float implGetProgress() const = 0;
        virtual ParamBlock const& implGetParams() const = 0;
        virtual std::span<OutputExtractor const> implGetOutputExtractors() const = 0;

        virtual void implRequestStop() = 0;
        virtual void implStop() = 0;

        virtual float implGetFixupScaleFactor() const = 0;
        virtual void implSetFixupScaleFactor(float) = 0;
    };
}
