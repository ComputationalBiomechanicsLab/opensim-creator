#include "ISimulatorUIAPI.h"

#include <libOpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <libOpenSimCreator/Documents/Simulation/ISimulation.h>
#include <libOpenSimCreator/Documents/Simulation/SimulationHelpers.h>

#include <liboscar/Platform/Log.h>
#include <liboscar/Platform/os.h>
#include <liboscar/Utils/Assertions.h>
#include <OpenSim/Simulation/Model/Model.h>

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
            prompt_user_for_file_save_location_add_extension_if_necessary("csv");
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

std::optional<std::filesystem::path> osc::ISimulatorUIAPI::tryPromptToSaveOutputsAsCSV(std::span<const OutputExtractor> outputs) const
{
    return TryExportOutputsToCSV(getSimulation(), outputs);
}

std::optional<std::filesystem::path> osc::ISimulatorUIAPI::tryPromptToSaveAllOutputsAsCSV(std::span<const OutputExtractor> outputs) const
{
    return TryExportOutputsToCSV(getSimulation(), outputs);
}
