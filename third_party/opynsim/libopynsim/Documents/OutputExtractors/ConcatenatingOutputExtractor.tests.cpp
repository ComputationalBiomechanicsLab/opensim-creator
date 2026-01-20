#include "ConcatenatingOutputExtractor.h"

#include <libopynsim/Documents/CustomComponents/BlankComponent.h>
#include <libopynsim/Documents/OutputExtractors/ConstantOutputExtractor.h>
#include <libopynsim/Documents/OutputExtractors/OutputExtractorDataType.h>
#include <libopynsim/Documents/StateViewWithMetadata.h>

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

    class BlankStateView final : public StateViewWithMetadata {
    private:
        const SimTK::State& implGetState() const { return m_State; }

        SimTK::State m_State;
    };
    const BlankStateView state;

    const auto output = concat.getValue<Vector2>(unusedRoot, state);

    ASSERT_EQ(output, Vector2(1.0f, 2.0f));
}
