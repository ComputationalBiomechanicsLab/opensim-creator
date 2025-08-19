#include "ConstantOutputExtractor.h"

#include <libopensimcreator/Documents/Simulation/SimulationReport.h>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Station.h>

using namespace osc;

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
    ASSERT_EQ(ConstantOutputExtractor("hello", Vector2{1.0f, 2.0f}).getOutputType(), OutputExtractorDataType::Vector2);
}

TEST(ConstantOutputExtractor, ReturnsAnExtractorThatEmitsTheProvidedValue)
{
    ConstantOutputExtractor coe("extractor", 1337.0f);
    SimulationReport report;  // the state doesn't actually need any information for this type of extractor
    OpenSim::Station component;  // it doesn't matter which type of component it is for this extractor

    ASSERT_EQ(coe.getValueFloat(component, report), 1337.0f);
}

TEST(ConstantOutputExtractor, ReturnsAnExectactorThatEmitsVector2sWhenProviedVector2s)
{
    ConstantOutputExtractor coe("extractor", Vector2{2.0f, 3.0f});
    SimulationReport report;  // the state doesn't actually need any information for this type of extractor
    OpenSim::Station component;  // it doesn't matter which type of component it is for this extractor

    ASSERT_EQ(coe.getValueVector2(component, report), Vector2(2.0f, 3.0f));
}
