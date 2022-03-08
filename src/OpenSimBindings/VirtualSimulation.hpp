#pragma once

#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/Utils/UID.hpp"

#include <nonstd/span.hpp>

#include <optional>
#include <vector>
#include <string>

namespace osc
{
    class SimulationReport;
    class Output;
    class ParamBlock;
}

namespace OpenSim
{
    class Model;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    // a virtual simulation could be backed by:
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

        virtual OpenSim::Model const& getModel() const = 0;

        virtual int getNumReports() = 0;
        virtual SimulationReport getSimulationReport(int reportIndex) = 0;

        virtual SimulationStatus getSimulationStatus() const = 0;
        virtual void requestStop() = 0;
        virtual void stop() = 0;
        virtual SimulationClock::time_point getSimulationCurTime() = 0;
        virtual SimulationClock::time_point getSimulationStartTime() const = 0;
        virtual SimulationClock::time_point getSimulationEndTime() const = 0;
        virtual ParamBlock const& getSimulationParams() const = 0;
        virtual nonstd::span<Output const> getOutputs() const = 0;
    };
}
