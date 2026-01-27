#include "concatenating_output_extractor.h"

#include <libopynsim/documents/custom_components/blank_component.h>
#include <libopynsim/documents/output_extractors/constant_output_extractor.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>
#include <libopynsim/documents/state_view_with_metadata.h>

#include <gtest/gtest.h>

using namespace osc;

// Basic functionality test: a `ConcatenatingOutputExtractor` should at least be able to
// concatenate two floating point outputs (#1025).
TEST(ConcatenatingOutputExtractor, hasExpectedOutputsWhenConcatenatingTwoFloatOutput)
{
    const SharedOutputExtractor lhs = make_output_extractor<ConstantOutputExtractor>("lhslabel", 1.0f);
    const SharedOutputExtractor rhs = make_output_extractor<ConstantOutputExtractor>("rhslabel", 2.0f);
    const ConcatenatingOutputExtractor concat{lhs, rhs};

    ASSERT_EQ(concat.getOutputType(), OutputExtractorDataType::Vector2);

    const BlankComponent unusedRoot;

    class BlankStateView final : public opyn::StateViewWithMetadata {
    private:
        const SimTK::State& implGetState() const { return m_State; }

        SimTK::State m_State;
    };
    const BlankStateView state;

    const auto output = concat.getValue<Vector2>(unusedRoot, state);

    ASSERT_EQ(output, Vector2(1.0f, 2.0f));
}
