#include "OpenSimCreator/Documents/OutputExtractors/ConstantOutputExtractor.h"

#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSim/Simulation/Model/Station.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(ConstantOutputExtractor, ReturnsProvidedName)
{
    ASSERT_EQ(ConstantOutputExtractor("hello", 1.0f).getName(), "hello");
}

TEST(ConstantOutputExtractor, HasTypeFloatWhenConstructedFromFloat)
{
    ASSERT_EQ(ConstantOutputExtractor("hello", 1.0f).getOutputType(), OutputExtractorDataType::Float);
}

TEST(ConstantOutputExtractor, HasTypeVec2WhenConstructedFromVec2)
{
    ASSERT_EQ(ConstantOutputExtractor("hello", Vec2{1.0f, 2.0f}).getOutputType(), OutputExtractorDataType::Vec2);
}

TEST(ConstantOutputExtractor, ReturnsAnExtractorThatEmitsTheProvidedValue)
{
    ConstantOutputExtractor coe("extractor", 1337.0f);
    SimulationReport report;  // the state doesn't actually need any information for this type of extractor
    OpenSim::Station component;  // it doesn't matter which type of component it is for this extractor

    ASSERT_EQ(coe.getValueFloat(component, report), 1337.0f);
}

TEST(ConstantOutputExtractor, ReturnsAnExectactorThatEmitsVec2sWhenProviedVec2s)
{
    ConstantOutputExtractor coe("extractor", Vec2{2.0f, 3.0f});
    SimulationReport report;  // the state doesn't actually need any information for this type of extractor
    OpenSim::Station component;  // it doesn't matter which type of component it is for this extractor

    ASSERT_EQ(coe.getValueVec2(component, report), Vec2(2.0f, 3.0f));
}
