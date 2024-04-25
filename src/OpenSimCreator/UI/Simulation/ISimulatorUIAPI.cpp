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
    // export a timeseries to a CSV file and return the filepath
    std::optional<std::filesystem::path> ExportTimeseriesToCSV(
        std::span<const float> times,
        std::span<const float> values,
        std::string_view header)
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

        fout << "time," << header << '\n';
        for (size_t i = 0, len = min(times.size(), values.size()); i < len; ++i) {
            fout << times[i] << ',' << values[i] << '\n';
        }

        if (not fout) {
            log_error("%s: error encountered while writing CSV data to file", csvPath.string().c_str());
            return std::nullopt;  // error writing
        }

        log_info("%: successfully wrote CSV data to output file", csvPath.string().c_str());

        return csvPath;
    }

    std::vector<float> PopulateFirstNNumericOutputValues(
        const OpenSim::Model& model,
        std::span<const SimulationReport> reports,
        const IOutputExtractor& output)
    {
        std::vector<float> rv;
        rv.reserve(reports.size());
        output.getValuesFloat(model, reports, [&rv](float v)
        {
            rv.push_back(v);
        });
        return rv;
    }

    std::vector<float> PopulateFirstNTimeValues(std::span<const SimulationReport> reports)
    {
        std::vector<float> times;
        times.reserve(reports.size());
        for (const SimulationReport& report : reports) {
            times.push_back(static_cast<float>(report.getState().getTime()));
        }
        return times;
    }

    std::optional<std::filesystem::path> TryExportNumericOutputToCSV(
        ISimulation& simulation,
        const IOutputExtractor& output)
    {
        OSC_ASSERT(output.getOutputType() == OutputExtractorDataType::Float);

        const std::vector<SimulationReport> reports = simulation.getAllSimulationReports();
        const std::vector<float> values = PopulateFirstNNumericOutputValues(*simulation.getModel(), reports, output);
        const std::vector<float> times = PopulateFirstNTimeValues(reports);

        return ExportTimeseriesToCSV(times, values, output.getName());
    }

    std::optional<std::filesystem::path> TryExportOutputsToCSV(
        const ISimulation& simulation,
        std::span<const OutputExtractor> outputs)
    {
        const std::vector<SimulationReport> reports = simulation.getAllSimulationReports();
        const std::vector<float> times = PopulateFirstNTimeValues(reports);

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

        // header line
        fout << "time";
        for (const OutputExtractor& o : outputs) {
            fout << ',' << o.getName();
        }
        fout << '\n';


        // data lines
        const auto guard = simulation.getModel();
        for (size_t i = 0; i < reports.size(); ++i) {
            fout << times.at(i);  // time column

            SimulationReport r = reports[i];
            for (const OutputExtractor& o : outputs) {
                fout << ',' << o.getValueFloat(*guard, r);
            }

            fout << '\n';
        }

        if (not fout) {
            log_warn("%s: encountered error while writing output data: some of the data may have been written, but maybe not all of it", csvPath.string().c_str());
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
