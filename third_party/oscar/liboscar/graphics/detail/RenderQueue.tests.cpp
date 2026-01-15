#include "RenderQueue.h"

#include <liboscar/graphics/geometries/ConeGeometry.h>
#include <liboscar/graphics/Mesh.h>
#include <liboscar/graphics/Material.h>
#include <liboscar/graphics/MaterialPropertyBlock.h>
#include <liboscar/graphics/materials/MeshBasicMaterial.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_metadata.h>
#include <liboscar/tests/testoscarconfig.h>
#include <liboscar/maths/Matrix4x4.h>

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
            metadata.set_organization_name(TESTOSCAR_ORGNAME_STRING);
            metadata.set_application_name(TESTOSCAR_APPNAME_STRING);
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

TEST_F(RenderQueueFixture, emplace_with_matrix_returns_reference_to_emplaced_data)
{
    RenderQueueEmplaceArgs args;

    RenderQueue render_queue;
    const auto ref = render_queue.emplace(args.mesh, args.transform, args.material, args.material_property_block, args.submesh_index);

    ASSERT_EQ(ref.mesh(), args.mesh);
    ASSERT_EQ(ref.model_matrix(), args.transform);
    ASSERT_EQ(ref.material(), args.material);
    ASSERT_EQ(ref.material_property_block(), args.material_property_block);
    ASSERT_EQ(ref.maybe_submesh_index(), args.submesh_index);
}

TEST_F(RenderQueueFixture, begin_returns_iterator_that_points_to_first_element)
{
    RenderQueueEmplaceArgs args;

    RenderQueue render_queue;
    render_queue.emplace(args.mesh, args.transform, args.material, args.material_property_block, args.submesh_index);

    const auto it = render_queue.begin();
    ASSERT_EQ(it->mesh(), args.mesh);
    ASSERT_EQ(it->model_matrix(), args.transform);
    ASSERT_EQ(it->material(), args.material);
    ASSERT_EQ(it->material_property_block(), args.material_property_block);
    ASSERT_EQ(it->maybe_submesh_index(), args.submesh_index);
}

TEST_F(RenderQueueFixture, begin_equals_end_with_empty_queue)
{
    const RenderQueue render_queue;
    ASSERT_EQ(render_queue.begin(), render_queue.end());
}

TEST_F(RenderQueueFixture, distance_between_begin_and_end_is_one_with_one_element)
{
    RenderQueueEmplaceArgs args;

    RenderQueue render_queue;
    render_queue.emplace(args.mesh, args.transform, args.material, args.material_property_block, args.submesh_index);
    ASSERT_EQ(std::distance(render_queue.begin(), render_queue.end()), 1);
}

TEST_F(RenderQueueFixture, is_permutable)
{
    // Required for sorting, partitioning, etc.
    static_assert(std::permutable<RenderQueue::iterator>);
}

TEST(RenderQueue, iterator_is_input_or_output_iterator)
{
    // Required by libstdc++, libc++
    static_assert(std::input_or_output_iterator<RenderQueue::iterator>);
    static_assert(std::semiregular<RenderQueue::iterator>);
    static_assert(std::sentinel_for<RenderQueue::iterator, RenderQueue::iterator>);
    static_assert(std::default_initializable<RenderQueue::iterator>);
    static_assert(std::constructible_from<RenderQueue::iterator>);
    static_assert(std::is_constructible_v<RenderQueue::iterator>);
}
