#include "SimulationHelpers.h"

#include <libopensimcreator/Documents/Simulation/ISimulation.h>
#include <libopensimcreator/Documents/Simulation/SimulationReport.h>

#include <libopynsim/Documents/OutputExtractors/OutputExtractor.h>
#include <liboscar/utils/enum_helpers.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <cstddef>
#include <ostream>
#include <span>

void osc::WriteOutputsAsCSV(
    const OpenSim::Component& root,
    std::span<const OutputExtractor> outputs,
    std::span<const SimulationReport> reports,
    std::ostream& out)
{
    // header line
    out << "time";
    for (const OutputExtractor& o : outputs) {
        static_assert(num_options<OutputExtractorDataType>() == 3);
        if (o.getOutputType() == OutputExtractorDataType::Vector2) {
            out << ',' << o.getName() << "/0";
            out << ',' << o.getName() << "/1";
        }
        else {
            out << ',' << o.getName();
        }
    }
    out << '\n';

    // data lines
    for (const SimulationReport& report : reports) {
        out << report.getState().getTime();  // time column
        for (const OutputExtractor& o : outputs) {
            static_assert(num_options<OutputExtractorDataType>() == 3);
            if (o.getOutputType() == OutputExtractorDataType::Vector2) {
                const auto v = o.getValue<Vector2>(root, report);
                out << ',' << v.x() << ',' << v.y();
            }
            else {
                out << ',' << o.getValue<float>(root, report);
            }
        }
        out << '\n';
    }
}
