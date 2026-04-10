#include "render_queue.h"

#include <liboscar/graphics/geometries/cone_geometry.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/material.h>
#include <liboscar/graphics/material_property_block.h>
#include <liboscar/graphics/materials/mesh_basic_material.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_metadata.h>
#include <liboscar/tests/oscar_tests_config.h>
#include <liboscar/maths/matrix4x4.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>

using namespace osc;
using namespace osc::detail;

namespace
{
    std::unique_ptr<App> g_material_app;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    class RenderQueueFixture : public ::testing::Test {
    protected:
        static void SetUpTestSuite()
        {
            AppMetadata metadata;
            metadata.set_organization_name(OSCAR_TESTS_ORGNAME_STRING);
            metadata.set_application_name(OSCAR_TESTS_APPNAME_STRING);
            g_material_app = std::make_unique<App>(metadata);
        }

        static void TearDownTestSuite()
        {
            g_material_app.reset();
        }
    };

    struct RenderQueueEmplaceArgs final {
        Mesh mesh = ConeGeometry{}.mesh();
        Matrix4x4 transform{3.7f};
        Material material = MeshBasicMaterial{};
        MaterialPropertyBlock material_property_block;
        size_t submesh_index = 3;
    };
}

TEST_F(RenderQueueFixture, is_default_constructible)
{
    [[maybe_unused]] const RenderQueue default_constructed;
}

TEST_F(RenderQueueFixture, emplace_with_matrix_returns_handle_to_emplaced_data)
{
    RenderQueueEmplaceArgs args;

    RenderQueue render_queue;
    const auto handle = render_queue.emplace(args.mesh, args.transform, args.material, args.material_property_block, args.submesh_index);

    ASSERT_EQ(render_queue.mesh(handle), args.mesh);
    ASSERT_EQ(render_queue.model_matrix(handle), args.transform);
    ASSERT_EQ(render_queue.material(handle), args.material);
    ASSERT_EQ(render_queue.material_property_block(handle), args.material_property_block);
    ASSERT_EQ(render_queue.maybe_submesh_index(handle), args.submesh_index);
}

TEST_F(RenderQueueFixture, handles_begin_returns_iterator_that_points_to_first_element)
{
    RenderQueueEmplaceArgs args;

    RenderQueue render_queue;
    render_queue.emplace(args.mesh, args.transform, args.material, args.material_property_block, args.submesh_index);

    const auto it = render_queue.handles().begin();
    ASSERT_EQ(render_queue.mesh(*it), args.mesh);
    ASSERT_EQ(render_queue.model_matrix(*it), args.transform);
    ASSERT_EQ(render_queue.material(*it), args.material);
    ASSERT_EQ(render_queue.material_property_block(*it), args.material_property_block);
    ASSERT_EQ(render_queue.maybe_submesh_index(*it), args.submesh_index);
}

TEST_F(RenderQueueFixture, handles_begin_equals_end_with_empty_queue)
{
    const RenderQueue render_queue;
    ASSERT_EQ(render_queue.handles().begin(), render_queue.handles().end());
}

TEST_F(RenderQueueFixture, distance_between_handles_begin_and_end_is_one_with_one_element)
{
    RenderQueueEmplaceArgs args;

    RenderQueue render_queue;
    render_queue.emplace(args.mesh, args.transform, args.material, args.material_property_block, args.submesh_index);
    ASSERT_EQ(std::distance(render_queue.handles().begin(), render_queue.handles().end()), 1);
}

TEST_F(RenderQueueFixture, handles_are_permutable)
{
    // Required for sorting, partitioning, etc.
    static_assert(requires (RenderQueue& render_queue) { { render_queue.handles().begin() } -> std::permutable; });
}
