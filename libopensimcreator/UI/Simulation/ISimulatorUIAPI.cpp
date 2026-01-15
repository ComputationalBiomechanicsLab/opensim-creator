#include "ISimulatorUIAPI.h"

#include <libopensimcreator/Documents/OutputExtractors/OutputExtractor.h>
#include <libopensimcreator/Documents/Simulation/ISimulation.h>
#include <libopensimcreator/Documents/Simulation/SimulationHelpers.h>

#include <liboscar/platform/app.h>
#include <liboscar/platform/log.h>
#include <liboscar/platform/os.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <utility>

using namespace osc;

namespace
{
    void TryExportOutputsToCSV(
        const ISimulation& simulation,
        std::span<const OutputExtractor> outputs,
        bool openInDefaultApp)
    {
        // Pre-write the CSV in-memory so that the asynchronous user prompt isn't
        // dependent on a bunch of state.
        std::stringstream ss;
        {
            const auto guard = simulation.getModel();
            WriteOutputsAsCSV(*guard, outputs, simulation.getAllSimulationReports(), ss);
        }

        // Asynchronously prompt the user to select a save location and write the CSV
        // to it. If requested, open it in the user's default application.
        App::upd().prompt_user_to_save_file_with_extension_async([openInDefaultApp, csv = std::move(ss).str()](std::optional<std::filesystem::path> p)
        {
            if (not p) {
                return;  // user cancelled out of the prompt
            }

            // Open+write output file
            {
                std::ofstream ofs{*p};
                if (not ofs) {
                    log_error("%s: error opening file for writing", p->string().c_str());
                    return;  // error opening output file for writing
                }

                ofs << csv;
            }

            if (openInDefaultApp) {
                open_file_in_os_default_application(*p);
            }
        }, "csv");
    }
}

void osc::ISimulatorUIAPI::tryPromptToSaveOutputsAsCSV(std::span<const OutputExtractor> outputs, bool openInDefaultApp) const
{
    TryExportOutputsToCSV(getSimulation(), outputs, openInDefaultApp);
}

void osc::ISimulatorUIAPI::tryPromptToSaveAllOutputsAsCSV(std::span<const OutputExtractor> outputs, bool openInDefaultApp) const
{
    TryExportOutputsToCSV(getSimulation(), outputs, openInDefaultApp);
}
