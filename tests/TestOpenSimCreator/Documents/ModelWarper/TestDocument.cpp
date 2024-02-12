#include <OpenSimCreator/Documents/ModelWarper/Document.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpPairing.h>
#include <gtest/gtest.h>

#include <cctype>
#include <filesystem>
#include <stdexcept>

using osc::mow::Document;

namespace
{
    std::filesystem::path GetFixturesDir()
    {
        auto p = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources/TestOpenSimCreator/Document/ModelWarper";
        p = std::filesystem::weakly_canonical(p);
        return p;
    }
}

TEST(ModelWarpingDocument, CanDefaultConstruct)
{
    ASSERT_NO_THROW({ Document{}; });
}

TEST(ModelWarpingDocument, CanConstructFromPathToOsim)
{
    ASSERT_NO_THROW({ Document{GetFixturesDir() / "blank.osim"}; });
}

TEST(ModelWarpingDocument, ConstructorThrowsIfGivenInvalidOsimPath)
{
    ASSERT_THROW({ Document{std::filesystem::path{"bs.osim"}}; }, std::exception);
}

TEST(ModelWarpingDocument, AfterConstructingFromBasicOsimFileTheReturnedModelContainsExpectedComponents)
{
    Document const doc{GetFixturesDir() / "onebody.osim"};
    doc.getModel().getComponent("bodyset/some_body");
}
