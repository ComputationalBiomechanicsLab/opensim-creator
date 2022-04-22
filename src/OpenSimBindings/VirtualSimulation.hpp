#pragma once

#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <nonstd/span.hpp>

#include <vector>

namespace osc
{
    class OutputExtractor;
    class ParamBlock;
}

namespace OpenSim
{
    class Model;
}

namespace osc
{
    // a virtual simulation could be backed by (e.g.):
    //
    // - a real "live" forward-dynamic simulation
    // - an .sto file
    //
    // the GUI code shouldn't care about the specifics - it's up to each concrete
    // implementation to ensure this API is obeyed w.r.t. multithreading etc.
    class VirtualSimulation {
    public:
        virtual ~VirtualSimulation() noexcept = default;

        // the reason why the model is mutex-guarded is because OpenSim has a bunch of
        // `const-` interfaces that are only "logically const" in a single-threaded
        // environment.
        //
        // this can lead to mayhem if (e.g.) the model is actually being mutated by
        // multiple threads concurrently
        virtual SynchronizedValueGuard<OpenSim::Model const> getModel() const = 0;

        virtual int getNumReports() const = 0;
        virtual SimulationReport getSimulationReport(int reportIndex) const = 0;
        virtual std::vector<SimulationReport> getAllSimulationReports() const = 0;

        virtual SimulationStatus getStatus() const = 0;
        virtual SimulationClock::time_point getCurTime() const = 0;
        virtual SimulationClock::time_point getStartTime() const = 0;
        virtual SimulationClock::time_point getEndTime() const = 0;
        virtual float getProgress() const = 0;
        virtual ParamBlock const& getParams() const = 0;
        virtual nonstd::span<OutputExtractor const> getOutputExtractors() const = 0;

        virtual void requestStop() = 0;
        virtual void stop() = 0;
    };
}
