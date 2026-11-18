#include "simulation_helpers.h"

#include <libopensimcreator/documents/simulation/simulation_report.h>

#include <gtest/gtest.h>
#include <libopynsim/documents/output_extractors/constant_output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/formats/csv.h>
#include <liboscar/utilities/string_helpers.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <array>
#include <sstream>
#include <string>

using namespace osc;

TEST(SimulationHelpers, WriteOutputsAsCSVWritesFloatDataCorrectly)
{
    OpenSim::Model model;
    opyn::initialize_model(model);
    opyn::initialize_state(model);

    const auto extractors = std::to_array({
        opyn::make_output_extractor<opyn::ConstantOutputExtractor>("dummy", 1337.0f),
    });

    const auto reports = std::to_array({
        SimulationReport{SimTK::State{model.getWorkingState()}},
    });

    std::stringstream out;
    write_outputs_as_csv(model, extractors, reports, out);

    const auto row0 = CSV::read_row(out);
    const auto row1 = CSV::read_row(out);
    ASSERT_TRUE(CSV::read_row(out)) << "trailing newline?";
    ASSERT_FALSE(CSV::read_row(out)) << "EOF";

    const std::vector<std::string> row0_expected = {"time", "dummy"};
    ASSERT_EQ(row0, row0_expected);
    const std::vector<std::string> row1_expected = {
        stream_to_string(0.0), stream_to_string(1337.0f)};
    ASSERT_EQ(row1, row1_expected);
}

TEST(SimulationHelpers, WriteOutputsAsCSVWritesVector2DataCorrectly)
{
    OpenSim::Model model;
    opyn::initialize_model(model);
    opyn::initialize_state(model);

    const auto extractors = std::to_array({
        opyn::make_output_extractor<opyn::ConstantOutputExtractor>("dummy", Vector2{3.0f, 2.0f}),
    });

    const auto reports = std::to_array({
        SimulationReport{SimTK::State{model.getWorkingState()}},
    });

    std::stringstream out;
    write_outputs_as_csv(model, extractors, reports, out);

    const auto row0 = CSV::read_row(out);
    const auto row1 = CSV::read_row(out);
    ASSERT_TRUE(CSV::read_row(out)) << "trailing newline?";
    ASSERT_FALSE(CSV::read_row(out)) << "EOF";

    const std::vector<std::string> row0_expected = {"time", "dummy/0", "dummy/1"};
    ASSERT_EQ(row0, row0_expected);
    const std::vector<std::string> row1_expected = {
        stream_to_string(0.0), stream_to_string(3.0f), stream_to_string(2.0f)};
    ASSERT_EQ(row1, row1_expected);
}
