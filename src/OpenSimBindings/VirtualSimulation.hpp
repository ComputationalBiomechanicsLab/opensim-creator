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

        // the reason some methods are non-const is because (e.g.) getting a report
        // may involve doing some sort of lazy computation with the underlying backend

        // the reason this is synchronized atm is because background threads may need
        // to potentially internally mutate a model (e.g. by realizing the state against
        // the model) - this is due to an aliasing bug in OpenSim's GeometryPath wrapping
        // impl.
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
