#pragma once

#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/Utils/UID.hpp"

#include <nonstd/span.hpp>

#include <chrono>
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
        virtual OpenSim::Model const& getModel() const = 0;
        virtual UID getModelVersion() const = 0;

        virtual int getNumReports() const = 0;
        virtual SimulationReport const& getSimulationReport(int reportIndex) = 0;  // note: might mutate model internally
        virtual SimTK::State const& getReportState(int reportIndex) = 0;  // note: might mutate model internally
        virtual int tryGetAllReportNumericValues(Output const&, std::vector<float>& appendOut) const = 0;
        virtual std::optional<std::string> tryGetOutputString(Output const&, int reportIndex) const = 0;

        // simulator state
        virtual SimulationStatus getSimulationStatus() const = 0;
        virtual void requestStop() = 0;
        virtual void stop() = 0;
        virtual std::chrono::duration<double> getSimulationEndTime() const = 0;
        virtual float getSimulationProgress() const = 0;
        virtual ParamBlock const& getSimulationParams() const = 0;
        virtual nonstd::span<Output const> getOutputs() const = 0;
    };
}
