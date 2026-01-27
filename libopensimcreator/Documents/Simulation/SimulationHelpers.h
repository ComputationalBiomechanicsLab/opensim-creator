#pragma once

#include <iosfwd>
#include <span>

namespace opyn { class SharedOutputExtractor; }
namespace OpenSim { class Component; }
namespace osc { class SimulationReport; }

namespace osc
{
    void WriteOutputsAsCSV(
        const OpenSim::Component&,
        std::span<const opyn::SharedOutputExtractor>,
        std::span<const SimulationReport>,
        std::ostream&
    );
}
