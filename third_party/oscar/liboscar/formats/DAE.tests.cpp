#include "DAE.h"

#include <liboscar/graphics/geometries/BoxGeometry.h>
#include <liboscar/graphics/Mesh.h>
#include <liboscar/graphics/scene/SceneDecoration.h>
#include <liboscar/tests/testoscarconfig.h>
#include <liboscar/utils/StringHelpers.h>

#include <gtest/gtest.h>

#include <sstream>

using namespace osc;

TEST(DAE, write_works_for_empty_scene)
{
    const DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};

    std::stringstream ss;
    DAE::write(ss, {}, metadata);

    ASSERT_FALSE(ss.str().empty());
}

TEST(DAE, write_works_for_nonempty_scene)
{
    const DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};

    const SceneDecoration decoration = {.mesh = BoxGeometry{}};

    std::stringstream ss;
    DAE::write(ss, {{decoration}}, metadata);

    ASSERT_FALSE(ss.str().empty());
}

TEST(DAE, write_set_author_writes_author_to_output)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};
    metadata.author = "TestThis";

    std::stringstream ss;
    DAE::write(ss, {}, metadata);

    ASSERT_TRUE(ss.str().contains(metadata.author));
}

TEST(DAE, write_set_authoring_tool_writes_authoring_tool_to_output)
{
    DAEMetadata metadata{TESTOSCAR_APPNAME_STRING, TESTOSCAR_APPNAME_STRING};
    metadata.authoring_tool = "TestThis";

    std::stringstream ss;
    DAE::write(ss, {}, metadata);

    ASSERT_TRUE(ss.str().contains(metadata.authoring_tool));
}
