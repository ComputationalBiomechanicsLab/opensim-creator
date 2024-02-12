#include <oscar/Formats/DAE.h>

#include <testoscar/testoscarconfig.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshGenerators.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Utils/StringHelpers.h>

#include <sstream>

using namespace osc;

TEST(DAE, WriteDecorationsAsDAEWorksForEmptyScene)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};

    std::stringstream ss;
    WriteDecorationsAsDAE(ss, {}, metadata);

    ASSERT_FALSE(ss.str().empty());
}

TEST(DAE, WriteDecorationsAsDAEWorksForNonEmptyScene)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};

    SceneDecoration const dec{.mesh = GenerateCubeMesh()};

    std::stringstream ss;
    WriteDecorationsAsDAE(ss, {{dec}}, metadata);

    ASSERT_FALSE(ss.str().empty());
}

TEST(DAE, SetAuthorWritesAuthorToOutput)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};
    metadata.author = "TestThis";

    std::stringstream ss;
    WriteDecorationsAsDAE(ss, {}, metadata);

    ASSERT_TRUE(Contains(ss.str(), metadata.author));
}

TEST(DAE, SetAuthoringToolsWritesAuthoringToolToOutput)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};
    metadata.authoringTool = "TestThis";

    std::stringstream ss;
    WriteDecorationsAsDAE(ss, {}, metadata);

    ASSERT_TRUE(Contains(ss.str(), metadata.authoringTool));
}
