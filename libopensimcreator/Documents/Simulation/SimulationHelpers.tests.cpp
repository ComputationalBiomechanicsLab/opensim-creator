#include "SimulationHelpers.h"

#include <libopensimcreator/Documents/Simulation/SimulationReport.h>

#include <gtest/gtest.h>
#include <libopynsim/documents/output_extractors/constant_output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/formats/csv.h>
#include <liboscar/utils/string_helpers.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <array>
#include <sstream>
#include <string>

using namespace osc;

TEST(SimulationHelpers, WriteOutputsAsCSVWritesFloatDataCorrectly)
{
    OpenSim::Model model;
    opyn::InitializeModel(model);
    opyn::InitializeState(model);

    const auto extractors = std::to_array({
        make_output_extractor<ConstantOutputExtractor>("dummy", 1337.0f),
    });

    const auto reports = std::to_array({
        SimulationReport{SimTK::State{model.getWorkingState()}}
    });

    std::stringstream out;
    WriteOutputsAsCSV(model, extractors,reports, out);

    const auto row0 = CSV::read_row(out);
    const auto row1 = CSV::read_row(out);
    ASSERT_TRUE(CSV::read_row(out)) << "trailing newline?";
    ASSERT_FALSE(CSV::read_row(out)) << "EOF";

    const std::vector<std::string> row0Expected = { "time", "dummy" };
    ASSERT_EQ(row0, row0Expected);
    const std::vector<std::string> row1Expected = { stream_to_string(0.0), stream_to_string(1337.0f) };
    ASSERT_EQ(row1, row1Expected);
}

TEST(SimulationHelpers, WriteOutputsAsCSVWritesVector2DataCorrectly)
{
    OpenSim::Model model;
    opyn::InitializeModel(model);
    opyn::InitializeState(model);

    const auto extractors = std::to_array({
        make_output_extractor<ConstantOutputExtractor>("dummy", Vector2{3.0f, 2.0f}),
    });

    const auto reports = std::to_array({
        SimulationReport{SimTK::State{model.getWorkingState()}}
    });

    std::stringstream out;
    WriteOutputsAsCSV(model, extractors,reports, out);

    const auto row0 = CSV::read_row(out);
    const auto row1 = CSV::read_row(out);
    ASSERT_TRUE(CSV::read_row(out)) << "trailing newline?";
    ASSERT_FALSE(CSV::read_row(out)) << "EOF";

    const std::vector<std::string> row0Expected = { "time", "dummy/0", "dummy/1" };
    ASSERT_EQ(row0, row0Expected);
    const std::vector<std::string> row1Expected = { stream_to_string(0.0), stream_to_string(3.0f), stream_to_string(2.0f) };
    ASSERT_EQ(row1, row1Expected);
}
