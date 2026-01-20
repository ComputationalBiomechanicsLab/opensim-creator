#pragma once

#include <iosfwd>
#include <span>

namespace OpenSim { class Component; }
namespace osc { class SharedOutputExtractor; }
namespace osc { class SimulationReport; }

namespace osc
{
    void WriteOutputsAsCSV(
        const OpenSim::Component&,
        std::span<const SharedOutputExtractor>,
        std::span<const SimulationReport>,
        std::ostream&
    );
}
