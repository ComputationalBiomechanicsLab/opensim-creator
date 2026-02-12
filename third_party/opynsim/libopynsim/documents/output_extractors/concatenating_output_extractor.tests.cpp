#include "concatenating_output_extractor.h"

#include <libopynsim/documents/custom_components/blank_component.h>
#include <libopynsim/documents/output_extractors/constant_output_extractor.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>
#include <libopynsim/documents/state_view_with_metadata.h>

#include <gtest/gtest.h>

using namespace opyn;

// Basic functionality test: a `ConcatenatingOutputExtractor` should at least be able to
// concatenate two floating point outputs (#1025).
TEST(ConcatenatingOutputExtractor, hasExpectedOutputsWhenConcatenatingTwoFloatOutput)
{
    const opyn::SharedOutputExtractor lhs = opyn::make_output_extractor<ConstantOutputExtractor>("lhslabel", 1.0f);
    const opyn::SharedOutputExtractor rhs = opyn::make_output_extractor<ConstantOutputExtractor>("rhslabel", 2.0f);
    const ConcatenatingOutputExtractor concat{lhs, rhs};

    ASSERT_EQ(concat.getOutputType(), opyn::OutputExtractorDataType::Vector2);

    const BlankComponent unusedRoot;

    class BlankStateView final : public opyn::StateViewWithMetadata {
    private:
        const SimTK::State& implGetState() const override { return m_State; }

        SimTK::State m_State;
    };
    const BlankStateView state;

    const auto output = concat.getValue<osc::Vector2>(unusedRoot, state);

    ASSERT_EQ(output, osc::Vector2(1.0f, 2.0f));
}
