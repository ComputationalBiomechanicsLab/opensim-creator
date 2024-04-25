#include "SimulationHelpers.h"

#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/ISimulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <Simbody.h>

#include <cstddef>
#include <ostream>
#include <span>

void osc::WriteOutputsAsCSV(
    std::span<const OutputExtractor> outputs,
    ISimulation const& simulation,
    std::ostream& out)
{
    // header line
    out << "time";
    for (const OutputExtractor& o : outputs) {
        out << ',' << o.getName();
    }
    out << '\n';


    // data lines
    const auto guard = simulation.getModel();
    for (size_t i = 0, len = simulation.getNumReports(); i < len; ++i) {
        const SimulationReport report = simulation.getSimulationReport(i);

        out << report.getState().getTime();  // time column
        for (const OutputExtractor& o : outputs) {
            out << ',' << o.getValueFloat(*guard, report);
        }

        out << '\n';
    }
}
