#include "SimulationHelpers.h"

#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/ISimulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Utils/EnumHelpers.h>
#include <Simbody.h>

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
        if (o.getOutputType() == OutputExtractorDataType::Vec2) {
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
            if (o.getOutputType() == OutputExtractorDataType::Vec2) {
                const Vec2 v = o.getValueVec2(root, report);
                out << ',' << v.x << ',' << v.y;
            }
            else {
                out << ',' << o.getValueFloat(root, report);
            }
        }
        out << '\n';
    }
}
