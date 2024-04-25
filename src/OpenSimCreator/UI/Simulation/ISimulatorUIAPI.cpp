#include "ISimulatorUIAPI.h"

#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/ISimulation.h>

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
        // try prompt user for save location
        const std::optional<std::filesystem::path> maybeCSVPath =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (not maybeCSVPath) {
            return std::nullopt;  // user probably cancelled out
        }
        const std::filesystem::path& csvPath = *maybeCSVPath;

        std::ofstream fout{csvPath};
        if (not fout) {
            log_error("%s: error opening file for writing", csvPath.string().c_str());
            return std::nullopt;  // error opening output file for writing
        }
        fout.exceptions(std::ios_base::badbit | std::ios_base::failbit);

        // header line
        fout << "time";
        for (const OutputExtractor& o : outputs) {
            fout << ',' << o.getName();
        }
        fout << '\n';


        // data lines
        const auto guard = simulation.getModel();
        for (size_t i = 0, len = simulation.getNumReports(); i < len; ++i) {
            const SimulationReport report = simulation.getSimulationReport(i);

            fout << report.getState().getTime();  // time column
            for (const OutputExtractor& o : outputs) {
                fout << ',' << o.getValueFloat(*guard, report);
            }

            fout << '\n';
        }

        return csvPath;
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

std::optional<std::filesystem::path> osc::ISimulatorUIAPI::tryPromptToSaveOutputsAsCSV(std::span<OutputExtractor const> outputs) const
{
    return TryExportOutputsToCSV(getSimulation(), outputs);
}

std::optional<std::filesystem::path> osc::ISimulatorUIAPI::tryPromptToSaveAllOutputsAsCSV() const
{
    return TryExportOutputsToCSV(getSimulation(), getAllUserOutputExtractors());
}
