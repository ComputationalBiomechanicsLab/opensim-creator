#include "OpenSimCreator/Documents/Simulation/SimulationHelpers.h"

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Documents/OutputExtractors/ConstantOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <oscar/Formats/CSV.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/StringHelpers.h>

#include <array>
#include <sstream>

using namespace osc;

TEST(SimulationHelpers, WriteOutputsAsCSVWritesFloatDataCorrectly)
{
    OpenSim::Model model;
    InitializeModel(model);
    InitializeState(model);

    const auto extractors = std::to_array({
        make_output_extractor<ConstantOutputExtractor>("dummy", 1337.0f),
    });

    const auto reports = std::to_array({
        SimulationReport{SimTK::State{model.getWorkingState()}}
    });

    std::stringstream out;
    WriteOutputsAsCSV(model, extractors,reports, out);

    const auto row0 = read_csv_row(out);
    const auto row1 = read_csv_row(out);
    ASSERT_TRUE(read_csv_row(out)) << "trailing newline?";
    ASSERT_FALSE(read_csv_row(out)) << "EOF";

    const std::vector<std::string> row0Expected = { "time", "dummy" };
    ASSERT_EQ(row0, row0Expected);
    const std::vector<std::string> row1Expected = { stream_to_string(0.0), stream_to_string(1337.0f) };
    ASSERT_EQ(row1, row1Expected);
}

TEST(SimulationHelpers, WriteOutputsAsCSVWritesVec2DataCorrectly)
{
    OpenSim::Model model;
    InitializeModel(model);
    InitializeState(model);

    const auto extractors = std::to_array({
        make_output_extractor<ConstantOutputExtractor>("dummy", Vec2{3.0f, 2.0f}),
    });

    const auto reports = std::to_array({
        SimulationReport{SimTK::State{model.getWorkingState()}}
    });

    std::stringstream out;
    WriteOutputsAsCSV(model, extractors,reports, out);

    const auto row0 = read_csv_row(out);
    const auto row1 = read_csv_row(out);
    ASSERT_TRUE(read_csv_row(out)) << "trailing newline?";
    ASSERT_FALSE(read_csv_row(out)) << "EOF";

    const std::vector<std::string> row0Expected = { "time", "dummy/0", "dummy/1" };
    ASSERT_EQ(row0, row0Expected);
    const std::vector<std::string> row1Expected = { stream_to_string(0.0), stream_to_string(3.0f), stream_to_string(2.0f) };
    ASSERT_EQ(row1, row1Expected);
}
