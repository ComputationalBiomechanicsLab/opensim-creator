#include "simulator_ui_api.h"

#include <libopensimcreator/documents/simulation/abstract_simulation.h>
#include <libopensimcreator/documents/simulation/simulation_helpers.h>

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
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
    void try_export_outputs_to_csv(
        const AbstractSimulation& simulation,
        std::span<const opyn::SharedOutputExtractor> outputs,
        bool open_in_default_app)
    {
        // Pre-write the CSV in-memory so that the asynchronous user prompt isn't
        // dependent on a bunch of state.
        std::stringstream ss;
        {
            const auto guard = simulation.get_model();
            write_outputs_as_csv(*guard, outputs, simulation.get_all_simulation_reports(), ss);
        }

        // Asynchronously prompt the user to select a save location and write the CSV
        // to it. If requested, open it in the user's default application.
        App::upd().prompt_user_to_save_file_with_extension_async(
            [open_in_default_app,
             csv = std::move(ss).str()](std::optional<std::filesystem::path> p) {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }

                // Open+write output file
                {
                    std::ofstream ofs{*p};
                    if (not ofs) {
                        log_error("{}: error opening file for writing", p->string());
                        return;  // error opening output file for writing
                    }

                    ofs << csv;
                }

                if (open_in_default_app) {
                    open_file_in_os_default_application(*p);
                }
            },
            "csv");
    }
}

void osc::SimulatorUIAPI::try_prompt_to_save_outputs_as_csv(
    std::span<const opyn::SharedOutputExtractor> outputs, bool open_in_default_app) const
{
    try_export_outputs_to_csv(get_simulation(), outputs, open_in_default_app);
}

void osc::SimulatorUIAPI::try_prompt_to_save_all_outputs_as_csv(
    std::span<const opyn::SharedOutputExtractor> outputs, bool open_in_default_app) const
{
    try_export_outputs_to_csv(get_simulation(), outputs, open_in_default_app);
}
