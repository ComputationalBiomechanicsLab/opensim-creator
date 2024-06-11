#include <oscar/Graphics/SubMeshDescriptor.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/MeshTopology.h>

#include <cstddef>

using namespace osc;

TEST(SubMeshDescriptor, CanConstructFromOffsetCountAndTopology)
{
    ASSERT_NO_THROW({ SubMeshDescriptor(0, 20, MeshTopology::Triangles); });
}

TEST(SubMeshDescriptor, BaseVertexIsZeroIfNotGivenViaCtor)
{
    ASSERT_EQ(SubMeshDescriptor(0, 20, MeshTopology::Triangles).base_vertex(), 0);
}

TEST(SubMeshDescriptor, GetIndexStartReturnsFirstCtorArgument)
{
    ASSERT_EQ(SubMeshDescriptor(0, 35, MeshTopology::Lines).index_start(), 0);
    ASSERT_EQ(SubMeshDescriptor(73, 35, MeshTopology::Lines).index_start(), 73);
}

TEST(SubMeshDescriptor, GetIndexCountReturnsSecondCtorArgument)
{
    ASSERT_EQ(SubMeshDescriptor(0, 2, MeshTopology::Lines).index_count(), 2);
    ASSERT_EQ(SubMeshDescriptor(73, 489, MeshTopology::Lines).index_count(), 489);
}

TEST(SubMeshDescriptor, GetTopologyReturnsThirdCtorArgument)
{
    ASSERT_EQ(SubMeshDescriptor(0, 2, MeshTopology::Lines).topology(), MeshTopology::Lines);
    ASSERT_EQ(SubMeshDescriptor(73, 489, MeshTopology::Triangles).topology(), MeshTopology::Triangles);
}

TEST(SubMeshDescriptor, BaseVertexReturnsFourthCtorArgument)
{
    ASSERT_EQ(SubMeshDescriptor(0, 2, MeshTopology::Lines, 3).base_vertex(), 3);
    ASSERT_EQ(SubMeshDescriptor(0, 2, MeshTopology::Lines, 7).base_vertex(), 7);
}

TEST(SubMeshDescriptor, CopiesCompareEqual)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor b{a};
    ASSERT_EQ(a, b);
}

TEST(SubMeshDescriptor, SeparatelyConstructedInstancesCompareEqual)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor b{0, 10, MeshTopology::Triangles};
    ASSERT_EQ(a, b);
}

TEST(SubMeshDescriptor, DifferentStartingOffsetsComparesNonEqual)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor b{5, 10, MeshTopology::Triangles};
    ASSERT_NE(a, b);
}

TEST(SubMeshDescriptor, DifferentIndexCountComparesNonEqual)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor b{0, 15, MeshTopology::Triangles};
    ASSERT_NE(a, b);
}

TEST(SubMeshDescriptor, DifferentTopologyComparesNonEqual)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor b{0, 10, MeshTopology::Lines};
    ASSERT_NE(a, b);
}

TEST(SubMeshDescriptor, SameBaseVertexComparesEqual)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles, 5};
    const SubMeshDescriptor b{0, 10, MeshTopology::Triangles, 5};
    ASSERT_EQ(a, b);
}

TEST(SubMeshDescriptor, DifferentBaseVertexComparesNonEqual)
{
    const SubMeshDescriptor a{0, 10, MeshTopology::Triangles, 5};
    const SubMeshDescriptor b{0, 10, MeshTopology::Triangles, 10};
    ASSERT_NE(a, b);
}
