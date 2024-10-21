#include <oscar/Graphics/SubMeshDescriptor.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/MeshTopology.h>

#include <cstddef>

using namespace osc;

TEST(SubMeshDescriptor, can_construct_from_offset_count_and_topology)
{
    ASSERT_NO_THROW({ SubMeshDescriptor(0, 20, MeshTopology::Triangles); });
}

TEST(SubMeshDescriptor, base_vertex_is_zero_if_not_provided_via_constructor)
{
    ASSERT_EQ(SubMeshDescriptor(0, 20, MeshTopology::Triangles).base_vertex(), 0);
}

TEST(SubMeshDescriptor, index_start_returns_first_constructor_argument)
{
    ASSERT_EQ(SubMeshDescriptor(0, 35, MeshTopology::Lines).index_start(), 0);
    ASSERT_EQ(SubMeshDescriptor(73, 35, MeshTopology::Lines).index_start(), 73);
}

TEST(SubMeshDescriptor, index_count_returns_second_constructor_argument)
{
    ASSERT_EQ(SubMeshDescriptor(0, 2, MeshTopology::Lines).index_count(), 2);
    ASSERT_EQ(SubMeshDescriptor(73, 489, MeshTopology::Lines).index_count(), 489);
}

TEST(SubMeshDescriptor, topology_returns_third_constructor_argument)
{
    ASSERT_EQ(SubMeshDescriptor(0, 2, MeshTopology::Lines).topology(), MeshTopology::Lines);
    ASSERT_EQ(SubMeshDescriptor(73, 489, MeshTopology::Triangles).topology(), MeshTopology::Triangles);
}

TEST(SubMeshDescriptor, base_vertex_returns_fourth_constructor_argument)
{
    ASSERT_EQ(SubMeshDescriptor(0, 2, MeshTopology::Lines, 3).base_vertex(), 3);
    ASSERT_EQ(SubMeshDescriptor(0, 2, MeshTopology::Lines, 7).base_vertex(), 7);
}

TEST(SubMeshDescriptor, compares_equal_to_copies)
{
    const SubMeshDescriptor original{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor copy{original};
    ASSERT_EQ(original, copy);
}

TEST(SubMeshDescriptor, separately_constructed_instances_compare_equal)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor b{0, 10, MeshTopology::Triangles};
    ASSERT_EQ(a, b);
}

TEST(SubMeshDescriptor, constructing_with_different_offsets_compares_not_equal)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor b{5, 10, MeshTopology::Triangles};
    ASSERT_NE(a, b);
}

TEST(SubMeshDescriptor, constructing_with_different_index_count_compares_not_equal)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor b{0, 15, MeshTopology::Triangles};
    ASSERT_NE(a, b);
}

TEST(SubMeshDescriptor, constructing_with_different_MeshTopology_compares_not_equal)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor b{0, 10, MeshTopology::Lines};
    ASSERT_NE(a, b);
}

TEST(SubMeshDescriptor, same_base_vertex_compares_equal)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles, 5};
    const SubMeshDescriptor b{0, 10, MeshTopology::Triangles, 5};
    ASSERT_EQ(a, b);
}

TEST(SubMeshDescriptor, different_base_vertex_compares_not_equal)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles, 5};
    const SubMeshDescriptor b{0, 10, MeshTopology::Triangles, 10};
    ASSERT_NE(a, b);
}
