#include "DAE.h"

#include <liboscar/Graphics/Geometries/BoxGeometry.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/testing/testoscarconfig.h>
#include <liboscar/Utils/StringHelpers.h>

#include <gtest/gtest.h>

#include <sstream>

using namespace osc;

TEST(write_as_dae, works_for_empty_scene)
{
    const DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};

    std::stringstream ss;
    write_as_dae(ss, {}, metadata);

    ASSERT_FALSE(ss.str().empty());
}

TEST(write_as_dae, works_for_nonempty_scene)
{
    const DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};

    const SceneDecoration decoration = {.mesh = BoxGeometry{}};

    std::stringstream ss;
    write_as_dae(ss, {{decoration}}, metadata);

    ASSERT_FALSE(ss.str().empty());
}

TEST(write_as_dae, set_author_writes_author_to_output)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};
    metadata.author = "TestThis";

    std::stringstream ss;
    write_as_dae(ss, {}, metadata);

    ASSERT_TRUE(contains(ss.str(), metadata.author));
}

TEST(write_as_dae, set_authoring_tool_writes_authoring_tool_to_output)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};
    metadata.authoring_tool = "TestThis";

    std::stringstream ss;
    write_as_dae(ss, {}, metadata);

    ASSERT_TRUE(contains(ss.str(), metadata.authoring_tool));
}
