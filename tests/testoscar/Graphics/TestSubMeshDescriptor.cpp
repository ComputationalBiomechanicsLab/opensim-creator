#include <oscar/Graphics/SubMeshDescriptor.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/MeshTopology.hpp>

#include <cstddef>

using osc::MeshTopology;
using osc::SubMeshDescriptor;

TEST(SubMeshDescriptor, CanConstructFromOffsetCountAndTopology)
{
    ASSERT_NO_THROW({ SubMeshDescriptor(0, 20, MeshTopology::Triangles); });
}

TEST(SubMeshDescriptor, GetIndexStartReturnsFirstCtorArgument)
{
    ASSERT_EQ(SubMeshDescriptor(0, 35, MeshTopology::Lines).getIndexStart(), 0);
    ASSERT_EQ(SubMeshDescriptor(73, 35, MeshTopology::Lines).getIndexStart(), 73);
}

TEST(SubMeshDescriptor, GetIndexCountReturnsSecondCtorArgument)
{
    ASSERT_EQ(SubMeshDescriptor(0, 2, MeshTopology::Lines).getIndexCount(), 2);
    ASSERT_EQ(SubMeshDescriptor(73, 489, MeshTopology::Lines).getIndexCount(), 489);
}

TEST(SubMeshDescriptor, GetTopologyReturnsThirdCtorArgument)
{
    ASSERT_EQ(SubMeshDescriptor(0, 2, MeshTopology::Lines).getTopology(), MeshTopology::Lines);
    ASSERT_EQ(SubMeshDescriptor(73, 489, MeshTopology::Triangles).getTopology(), MeshTopology::Triangles);
}

TEST(SubMeshDescriptor, CopiesCompareEqual)
{
    SubMeshDescriptor const a{0, 10, MeshTopology::Triangles};
    SubMeshDescriptor const b{a};
    ASSERT_EQ(a, b);
}

TEST(SubMeshDescriptor, SeparatelyConstructedInstancesCompareEqual)
{
    SubMeshDescriptor const a{0, 10, MeshTopology::Triangles};
    SubMeshDescriptor const b{0, 10, MeshTopology::Triangles};
    ASSERT_EQ(a, b);
}

TEST(SubMeshDescriptor, DifferentStartingOffsetsComparesNonEqual)
{
    SubMeshDescriptor const a{0, 10, MeshTopology::Triangles};
    SubMeshDescriptor const b{5, 10, MeshTopology::Triangles};
    ASSERT_NE(a, b);
}

TEST(SubMeshDescriptor, DifferentIndexCountComparesNonEqual)
{
    SubMeshDescriptor const a{0, 10, MeshTopology::Triangles};
    SubMeshDescriptor const b{0, 15, MeshTopology::Triangles};
    ASSERT_NE(a, b);
}

TEST(SubMeshDescriptor, DifferentTopologyComparesNonEqual)
{
    SubMeshDescriptor const a{0, 10, MeshTopology::Triangles};
    SubMeshDescriptor const b{0, 10, MeshTopology::Lines};
    ASSERT_NE(a, b);
}
