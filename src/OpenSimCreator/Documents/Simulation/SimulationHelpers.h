#pragma once

#include <iosfwd>
#include <span>

namespace osc { class ISimulation; }
namespace osc { class OutputExtractor; }

namespace osc
{
    void WriteOutputsAsCSV(
        std::span<const OutputExtractor>,
        ISimulation const&,
        std::ostream&
    );
}
