#include "src/Formats/DAE.hpp"

#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Utils/Algorithms.hpp"

#include <gtest/gtest.h>

#include <sstream>

TEST(DAE, WriteDecorationsAsDAEWorksForEmptyScene)
{
    std::stringstream ss;
    osc::WriteDecorationsAsDAE({}, ss);

    ASSERT_FALSE(ss.str().empty());
}

TEST(DAE, WriteDecorationsAsDAEWorksForNonEmptyScene)
{
    osc::SceneDecoration dec{osc::GenCube()};

    std::stringstream ss;
    osc::WriteDecorationsAsDAE({&dec, 1}, ss);

    ASSERT_FALSE(ss.str().empty());
}

TEST(DAE, SetAuthorWritesAuthorToOutput)
{
    osc::DAEMetadata metadata;
    metadata.author = "TestThis";

    std::stringstream ss;
    osc::WriteDecorationsAsDAE({}, ss, metadata);

    ASSERT_TRUE(osc::ContainsSubstring(ss.str(), metadata.author));
}

TEST(DAE, SetAuthoringToolsWritesAuthoringToolToOutput)
{
    osc::DAEMetadata metadata;
    metadata.authoringTool = "TestThis";

    std::stringstream ss;
    osc::WriteDecorationsAsDAE({}, ss, metadata);

    ASSERT_TRUE(osc::ContainsSubstring(ss.str(), metadata.authoringTool));
}
