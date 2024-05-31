#include <oscar/Formats/DAE.h>

#include <testoscar/testoscarconfig.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Geometries/BoxGeometry.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Utils/StringHelpers.h>

#include <sstream>

using namespace osc;

TEST(write_as_dae, WorksForEmptyScene)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};

    std::stringstream ss;
    write_as_dae(ss, {}, metadata);

    ASSERT_FALSE(ss.str().empty());
}

TEST(write_as_dae, WorksForNonEmptyScene)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};

    const SceneDecoration decoration{.mesh = BoxGeometry{2.0f, 2.0f, 2.0f}};

    std::stringstream ss;
    write_as_dae(ss, {{decoration}}, metadata);

    ASSERT_FALSE(ss.str().empty());
}

TEST(write_as_dae, SetAuthorWritesAuthorToOutput)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};
    metadata.author = "TestThis";

    std::stringstream ss;
    write_as_dae(ss, {}, metadata);

    ASSERT_TRUE(contains(ss.str(), metadata.author));
}

TEST(write_as_dae, SetAuthoringToolsWritesAuthoringToolToOutput)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};
    metadata.authoring_tool = "TestThis";

    std::stringstream ss;
    write_as_dae(ss, {}, metadata);

    ASSERT_TRUE(contains(ss.str(), metadata.authoring_tool));
}
