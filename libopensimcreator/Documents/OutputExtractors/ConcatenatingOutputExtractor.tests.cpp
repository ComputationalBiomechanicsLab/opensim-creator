#include "ConcatenatingOutputExtractor.h"

#include <libopensimcreator/Documents/OutputExtractors/ConstantOutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataType.h>
#include <libopensimcreator/Documents/Simulation/SimulationReport.h>
#include <libopynsim/Documents/CustomComponents/BlankComponent.h>

#include <gtest/gtest.h>

using namespace osc;

// Basic functionality test: a `ConcatenatingOutputExtractor` should at least be able to
// concatenate two floating point outputs (#1025).
TEST(ConcatenatingOutputExtractor, hasExpectedOutputsWhenConcatenatingTwoFloatOutput)
{
    const OutputExtractor lhs = make_output_extractor<ConstantOutputExtractor>("lhslabel", 1.0f);
    const OutputExtractor rhs = make_output_extractor<ConstantOutputExtractor>("rhslabel", 2.0f);
    const ConcatenatingOutputExtractor concat{lhs, rhs};

    ASSERT_EQ(concat.getOutputType(), OutputExtractorDataType::Vector2);

    const BlankComponent unusedRoot;
    const SimulationReport report;

    const Vector2 output = concat.getValueVector2(unusedRoot, report);

    ASSERT_EQ(output, Vector2(1.0f, 2.0f));
}
