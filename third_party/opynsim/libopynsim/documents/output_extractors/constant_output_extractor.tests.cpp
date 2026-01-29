#include "constant_output_extractor.h"

#include <gtest/gtest.h>
#include <libopynsim/documents/state_view_with_metadata.h>
#include <OpenSim/Simulation/Model/Station.h>

using namespace opyn;

namespace
{
    // Represents a view of a blank `SimTK::State` (can be handy for testing).
    class BlankStateView final : public StateViewWithMetadata {
    private:
        const SimTK::State& implGetState() const final { return m_State; }
        SimTK::State m_State;
    };
}

TEST(ConstantOutputExtractor, ReturnsProvidedName)
{
    ASSERT_EQ(ConstantOutputExtractor("hello", 1.0f).getName(), "hello");
}

TEST(ConstantOutputExtractor, HasTypeFloatWhenConstructedFromFloat)
{
    ASSERT_EQ(ConstantOutputExtractor("hello", 1.0f).getOutputType(), OutputExtractorDataType::Float);
}

TEST(ConstantOutputExtractor, HasTypeVector2WhenConstructedFromVector2)
{
    ASSERT_EQ(ConstantOutputExtractor("hello", osc::Vector2{1.0f, 2.0f}).getOutputType(), OutputExtractorDataType::Vector2);
}

TEST(ConstantOutputExtractor, ReturnsAnExtractorThatEmitsTheProvidedValue)
{
    ConstantOutputExtractor coe("extractor", 1337.0f);
    BlankStateView state;  // the state doesn't actually need any information for this type of extractor
    OpenSim::Station component;  // it doesn't matter which type of component it is for this extractor

    ASSERT_EQ(coe.getValue<float>(component, state), 1337.0f);
}

TEST(ConstantOutputExtractor, ReturnsAnExectactorThatEmitsVector2sWhenProviedVector2s)
{
    ConstantOutputExtractor coe("extractor", osc::Vector2{2.0f, 3.0f});
    BlankStateView state;  // the state doesn't actually need any information for this type of extractor
    OpenSim::Station component;  // it doesn't matter which type of component it is for this extractor

    ASSERT_EQ(coe.getValue<osc::Vector2>(component, state), osc::Vector2(2.0f, 3.0f));
}
