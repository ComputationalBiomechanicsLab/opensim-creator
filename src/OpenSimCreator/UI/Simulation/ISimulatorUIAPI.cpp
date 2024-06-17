#include "ISimulatorUIAPI.h"

#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/ISimulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationHelpers.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/Utils/Assertions.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>

using namespace osc;

namespace
{
    std::optional<std::filesystem::path> TryExportOutputsToCSV(
        const ISimulation& simulation,
        std::span<const OutputExtractor> outputs)
    {
        // prompt user for save location
        std::optional<std::filesystem::path> path =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
        if (not path) {
            return std::nullopt;  // user probably cancelled out
        }

        // open output file
        std::ofstream fout{*path};
        if (not fout) {
            log_error("%s: error opening file for writing", path->string().c_str());
            return std::nullopt;  // error opening output file for writing
        }
        fout.exceptions(std::ios_base::badbit | std::ios_base::failbit);

        // write output
        const auto guard = simulation.getModel();
        WriteOutputsAsCSV(*guard, outputs, simulation.getAllSimulationReports(), fout);

        return path;
    }
}

std::vector<OutputExtractor> osc::ISimulatorUIAPI::getAllUserOutputExtractors() const
{
    int nOutputs = getNumUserOutputExtractors();

    std::vector<OutputExtractor> rv;
    rv.reserve(nOutputs);
    for (int i = 0; i < nOutputs; ++i) {
        rv.push_back(getUserOutputExtractor(i));
    }
    return rv;
}

std::optional<std::filesystem::path> osc::ISimulatorUIAPI::tryPromptToSaveOutputsAsCSV(std::span<const OutputExtractor> outputs) const
{
    return TryExportOutputsToCSV(getSimulation(), outputs);
}

std::optional<std::filesystem::path> osc::ISimulatorUIAPI::tryPromptToSaveAllOutputsAsCSV() const
{
    return TryExportOutputsToCSV(getSimulation(), getAllUserOutputExtractors());
}
