#include "oscar/Formats/DAE.hpp"

#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/SceneDecoration.hpp"
#include "oscar/Utils/StringHelpers.hpp"

#include <gtest/gtest.h>

#include <sstream>

TEST(DAE, WriteDecorationsAsDAEWorksForEmptyScene)
{
    std::stringstream ss;
    osc::WriteDecorationsAsDAE(ss, {});

    ASSERT_FALSE(ss.str().empty());
}

TEST(DAE, WriteDecorationsAsDAEWorksForNonEmptyScene)
{
    osc::SceneDecoration dec{osc::GenCube()};

    std::stringstream ss;
    osc::WriteDecorationsAsDAE(ss, {&dec, 1});

    ASSERT_FALSE(ss.str().empty());
}

TEST(DAE, SetAuthorWritesAuthorToOutput)
{
    osc::DAEMetadata metadata;
    metadata.author = "TestThis";

    std::stringstream ss;
    osc::WriteDecorationsAsDAE(ss, {}, metadata);

    ASSERT_TRUE(osc::ContainsSubstring(ss.str(), metadata.author));
}

TEST(DAE, SetAuthoringToolsWritesAuthoringToolToOutput)
{
    osc::DAEMetadata metadata;
    metadata.authoringTool = "TestThis";

    std::stringstream ss;
    osc::WriteDecorationsAsDAE(ss, {}, metadata);

    ASSERT_TRUE(osc::ContainsSubstring(ss.str(), metadata.authoringTool));
}
