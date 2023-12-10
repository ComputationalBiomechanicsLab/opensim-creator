#include <oscar/Graphics/Mesh.hpp>

#include <testoscar/TestingHelpers.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Graphics/SubMeshDescriptor.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Quat.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <oscar/Utils/Concepts.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <sstream>
#include <utility>
#include <vector>

using osc::testing::ContainersEqual;
using osc::testing::GenerateTriangleVerts;
using osc::testing::GenerateVec2;
using osc::testing::GenerateVec3;
using osc::AABB;
using osc::Color;
using osc::ContiguousContainer;
using osc::Deg2Rad;
using osc::Identity;
using osc::Mat4;
using osc::Mesh;
using osc::MeshTopology;
using osc::Quat;
using osc::SubMeshDescriptor;
using osc::ToMat4;
using osc::Transform;
using osc::Vec2;
using osc::Vec3;
using osc::Vec4;

TEST(Mesh, CanBeDefaultConstructed)
{
    Mesh mesh;
}

TEST(Mesh, CanBeCopyConstructed)
{
    Mesh m;
    Mesh{m};
}

TEST(Mesh, CanBeMoveConstructed)
{
    Mesh m1;
    Mesh m2{std::move(m1)};
}

TEST(Mesh, CanBeCopyAssigned)
{
    Mesh m1;
    Mesh m2;

    m1 = m2;
}

TEST(Mesh, CanBeMoveAssigned)
{
    Mesh m1;
    Mesh m2;

    m1 = std::move(m2);
}

TEST(Mesh, CanGetTopology)
{
    Mesh m;

    m.getTopology();
}

TEST(Mesh, GetTopologyDefaultsToTriangles)
{
    Mesh m;

    ASSERT_EQ(m.getTopology(), MeshTopology::Triangles);
}

TEST(Mesh, SetTopologyCausesGetTopologyToUseSetValue)
{
    Mesh m;
    MeshTopology topography = MeshTopology::Lines;

    ASSERT_NE(m.getTopology(), MeshTopology::Lines);

    m.setTopology(topography);

    ASSERT_EQ(m.getTopology(), topography);
}

TEST(Mesh, SetTopologyCausesCopiedMeshTobeNotEqualToInitialMesh)
{
    Mesh m;
    Mesh copy{m};
    MeshTopology topology = MeshTopology::Lines;

    ASSERT_EQ(m, copy);
    ASSERT_NE(copy.getTopology(), topology);

    copy.setTopology(topology);

    ASSERT_NE(m, copy);
}

TEST(Mesh, GetNumVertsInitiallyEmpty)
{
    ASSERT_EQ(Mesh{}.getNumVerts(), 0);
}

TEST(Mesh, Assigning3VertsMakesGetNumVertsReturn3Verts)
{
    Mesh m;
    m.setVerts({{Vec3{}, Vec3{}, Vec3{}}});
    ASSERT_EQ(m.getNumVerts(), 3);
}

TEST(Mesh, HasVertsInitiallyfalse)
{
    ASSERT_FALSE(Mesh{}.hasVerts());
}

TEST(Mesh, HasVertsTrueAfterSettingVerts)
{
    Mesh m;
    m.setVerts({{Vec3{}, Vec3{}, Vec3{}}});
    ASSERT_TRUE(m.hasVerts());
}

TEST(Mesh, GetVertsReturnsEmptyVertsOnDefaultConstruction)
{
    Mesh m;
    ASSERT_TRUE(m.getVerts().empty());
}

TEST(Mesh, SetVertsMakesGetCallReturnVerts)
{
    Mesh m;
    std::vector<Vec3> verts = GenerateTriangleVerts();

    ASSERT_FALSE(ContainersEqual(m.getVerts(), verts));
}

TEST(Mesh, SetVertsCausesCopiedMeshToNotBeEqualToInitialMesh)
{
    Mesh m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.setVerts(GenerateTriangleVerts());

    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformVertsMakesGetCallReturnVerts)
{
    Mesh m;

    // generate "original" verts
    std::vector<Vec3> originalVerts = GenerateTriangleVerts();

    // create "transformed" version of the verts
    std::vector<Vec3> newVerts;
    newVerts.reserve(originalVerts.size());
    for (Vec3 const& v : originalVerts)
    {
        newVerts.push_back(v + 1.0f);
    }

    // sanity check that `setVerts` works as expected
    ASSERT_FALSE(m.hasVerts());
    m.setVerts(originalVerts);
    ASSERT_TRUE(ContainersEqual(m.getVerts(), originalVerts));

    // the verts passed to `transformVerts` should match those returned by getVerts
    std::vector<Vec3> vertsPassedToTransformVerts;
    m.transformVerts([&vertsPassedToTransformVerts](Vec3& v) { vertsPassedToTransformVerts.push_back(v); });
    ASSERT_EQ(vertsPassedToTransformVerts, originalVerts);

    // applying the transformation should return the transformed verts
    m.transformVerts([&newVerts, i = 0](Vec3& v) mutable
    {
        v = newVerts.at(i++);
    });
    ASSERT_TRUE(ContainersEqual(m.getVerts(), newVerts));
}

TEST(Mesh, TransformVertsCausesTransformedMeshToNotBeEqualToInitialMesh)
{
    Mesh m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transformVerts([](Vec3&) {});  // noop transform also triggers this (meshes aren't value-comparable)

    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformVertsWithTransformAppliesTransformToVerts)
{
    // create appropriate transform
    Transform t;
    t.scale *= 0.25f;
    t.position = {1.0f, 0.25f, 0.125f};
    t.rotation = Quat{Vec3{Deg2Rad(90.0f), 0.0f, 0.0f}};

    // generate "original" verts
    std::vector<Vec3> originalVerts = GenerateTriangleVerts();

    // precompute "expected" verts
    std::vector<Vec3> expectedVerts;
    expectedVerts.reserve(originalVerts.size());
    for (Vec3& v : originalVerts)
    {
        expectedVerts.push_back(t * v);
    }

    // create mesh with "original" verts
    Mesh m;
    m.setVerts(originalVerts);

    // then apply the transform
    m.transformVerts(t);

    // the mesh's verts should match expectations
    ASSERT_TRUE(ContainersEqual(m.getVerts(), expectedVerts));
}

TEST(Mesh, TransformVertsWithTransformCausesTransformedMeshToNotBeEqualToInitialMesh)
{
    Mesh m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transformVerts(Transform{});  // noop transform also triggers this (meshes aren't value-comparable)

    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformVertsWithMat4AppliesTransformToVerts)
{
    Mat4 const mat = ToMat4(Transform{
        .scale = Vec3{0.25f},
        .rotation = Quat{Vec3{Deg2Rad(90.0f), 0.0f, 0.0f}},
        .position = {1.0f, 0.25f, 0.125f},
    });

    // generate "original" verts
    std::vector<Vec3> originalVerts = GenerateTriangleVerts();

    // precompute "expected" verts
    std::vector<Vec3> expectedVerts;
    expectedVerts.reserve(originalVerts.size());
    for (Vec3& v : originalVerts)
    {
        expectedVerts.emplace_back(Vec3{mat * Vec4{v, 1.0f}});
    }

    // create mesh with "original" verts
    Mesh m;
    m.setVerts(originalVerts);

    // then apply the transform
    m.transformVerts(mat);

    // the mesh's verts should match expectations
    ASSERT_TRUE(ContainersEqual(m.getVerts(), expectedVerts));
}

TEST(Mesh, TransformVertsWithMat4CausesTransformedMeshToNotBeEqualToInitialMesh)
{
    Mesh m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transformVerts(Identity<Mat4>());  // noop transform also triggers this (meshes aren't value-comparable)

    ASSERT_NE(m, copy);
}

TEST(Mesh, HasNormalsReturnsFalseForNewlyConstructedMesh)
{
    ASSERT_FALSE(Mesh{}.hasNormals());
}

TEST(Mesh, AssigningNormalsMakesHasNormalsReturnTrue)
{
    Mesh m;
    m.setNormals({{Vec3{}, Vec3{}, Vec3{}}});
    ASSERT_TRUE(m.hasNormals());
}

TEST(Mesh, ClearingMeshClearsHasNormals)
{
    Mesh m;
    m.setNormals({{Vec3{}, Vec3{}, Vec3{}}});
    ASSERT_TRUE(m.hasNormals());
    m.clear();
    ASSERT_FALSE(m.hasNormals());
}

TEST(Mesh, HasNormalsReturnsFalseIfOnlyAssigningVerts)
{
    Mesh m;
    m.setVerts({{Vec3{}, Vec3{}, Vec3{}}});
    ASSERT_FALSE(m.hasNormals());
}

TEST(Mesh, GetNormalsReturnsEmptyOnDefaultConstruction)
{
    Mesh m;
    ASSERT_TRUE(m.getNormals().empty());
}

TEST(Mesh, SetNormalsMakesGetCallReturnSuppliedData)
{
    Mesh m;
    std::vector<Vec3> normals = {GenerateVec3(), GenerateVec3(), GenerateVec3()};

    ASSERT_TRUE(m.getNormals().empty());

    m.setNormals(normals);

    ASSERT_TRUE(ContainersEqual(m.getNormals(), normals));
}

TEST(Mesh, SetNormalsCausesCopiedMeshToNotBeEqualToInitialMesh)
{
    Mesh m;
    Mesh copy{m};
    std::vector<Vec3> normals = {GenerateVec3(), GenerateVec3(), GenerateVec3()};

    ASSERT_EQ(m, copy);

    copy.setNormals(normals);

    ASSERT_NE(m, copy);
}

TEST(Mesh, HasTexCoordsReturnsFalseForDefaultConstructedMesh)
{
    ASSERT_FALSE(Mesh{}.hasNormals());
}

TEST(Mesh, HasTexCoordsReturnsTrueAfterAssingingTextCoords)
{
    Mesh m;
    m.setTexCoords({{Vec2{}, Vec2{}}});
    ASSERT_TRUE(m.hasTexCoords());
}

TEST(Mesh, GetTexCoordsReturnsEmptyOnDefaultConstruction)
{
    Mesh m;
    ASSERT_TRUE(m.getTexCoords().empty());
}

TEST(Mesh, SetTexCoordsCausesGetToReturnSuppliedData)
{
    Mesh m;
    std::vector<Vec2> coords = {GenerateVec2(), GenerateVec2(), GenerateVec2()};

    ASSERT_TRUE(m.getTexCoords().empty());

    m.setTexCoords(coords);

    ASSERT_TRUE(ContainersEqual(m.getTexCoords(), coords));
}

TEST(Mesh, SetTexCoordsCausesCopiedMeshToNotBeEqualToInitialMesh)
{
    Mesh m;
    Mesh copy{m};
    std::vector<Vec2> coords = {GenerateVec2(), GenerateVec2(), GenerateVec2()};

    ASSERT_EQ(m, copy);

    copy.setTexCoords(coords);

    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformTexCoordsAppliesTransformToTexCoords)
{
    Mesh m;
    std::vector<Vec2> coords = {GenerateVec2(), GenerateVec2(), GenerateVec2()};

    m.setTexCoords(coords);

    ASSERT_TRUE(ContainersEqual(m.getTexCoords(), coords));

    auto const transformer = [](Vec2 v)
    {
        return 0.287f * v;  // arbitrary mutation
    };

    // mutate mesh
    m.transformTexCoords([&transformer](Vec2& uv) { uv = transformer(uv); });

    // perform equivalent mutation for comparison
    std::transform(coords.begin(), coords.end(), coords.begin(), transformer);

    ASSERT_TRUE(ContainersEqual(m.getTexCoords(), coords));
}

TEST(Mesh, GetColorsInitiallyReturnsEmptySpan)
{
    Mesh m;
    ASSERT_TRUE(m.getColors().empty());
}

TEST(Mesh, SetColorsFollowedByGetColorsReturnsColors)
{
    Mesh m;
    std::array<Color, 3> colors{};

    m.setColors(colors);

    auto rv = m.getColors();
    ASSERT_EQ(rv.size(), colors.size());
}

TEST(Mesh, GetTangentsInitiallyReturnsEmptySpan)
{
    Mesh m;
    ASSERT_TRUE(m.getTangents().empty());
}

TEST(Mesh, SetTangentsFollowedByGetTangentsReturnsTangents)
{
    Mesh m;
    std::array<Vec4, 5> tangents{};

    m.setTangents(tangents);
    ASSERT_EQ(m.getTangents().size(), tangents.size());
}

TEST(Mesh, GetNumIndicesReturnsZeroOnDefaultConstruction)
{
    Mesh m;
    ASSERT_EQ(m.getIndices().size(), 0);
}

TEST(Mesh, GetBoundsReturnsEmptyBoundsOnInitialization)
{
    Mesh m;
    AABB empty{};
    ASSERT_EQ(m.getBounds(), empty);
}

TEST(Mesh, GetBoundsReturnsEmptyForMeshWithUnindexedVerts)
{
    auto const pyramid = std::to_array<Vec3>(
    {
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
        { 0.0f,  0.0f, 1.0f},  // tip
    });

    Mesh m;
    m.setVerts(pyramid);
    AABB empty{};
    ASSERT_EQ(m.getBounds(), empty);
}

TEST(Mesh, GetBooundsReturnsNonemptyForIndexedVerts)
{
    auto const pyramid = std::to_array<Vec3>(
    {
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
    });
    auto const pyramidIndices = std::to_array<uint16_t>({0, 1, 2});

    Mesh m;
    m.setVerts(pyramid);
    m.setIndices(pyramidIndices);
    AABB expected = osc::AABBFromVerts(pyramid);
    ASSERT_EQ(m.getBounds(), expected);
}

TEST(Mesh, CanBeComparedForEquality)
{
    Mesh m1;
    Mesh m2;

    (void)(m1 == m2);  // just ensure the expression compiles
}

TEST(Mesh, CopiesAreEqual)
{
    Mesh m;
    Mesh copy{m};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(m, copy);
}

TEST(Mesh, CanBeComparedForNotEquals)
{
    Mesh m1;
    Mesh m2;

    (void)(m1 != m2);  // just ensure the expression compiles
}

TEST(Mesh, CanBeWrittenToOutputStreamForDebugging)
{
    Mesh m;
    std::stringstream ss;

    ss << m;

    ASSERT_FALSE(ss.str().empty());
}

TEST(Mesh, GetSubMeshCountReturnsZeroForDefaultConstructedMesh)
{
    ASSERT_EQ(Mesh{}.getSubMeshCount(), 0);
}

TEST(Mesh, GetSubMeshCountReturnsZeroForMeshWithSomeData)
{
    auto const pyramid = std::to_array<Vec3>(
    {
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
    });
    auto const pyramidIndices = std::to_array<uint16_t>({0, 1, 2});

    Mesh m;
    m.setVerts(pyramid);
    m.setIndices(pyramidIndices);

    ASSERT_EQ(m.getSubMeshCount(), 0);
}

TEST(Mesh, PushSubMeshDescriptorMakesGetMeshSubCountIncrease)
{
    Mesh m;
    ASSERT_EQ(m.getSubMeshCount(), 0);
    m.pushSubMeshDescriptor(SubMeshDescriptor{0, 10, MeshTopology::Triangles});
    ASSERT_EQ(m.getSubMeshCount(), 1);
    m.pushSubMeshDescriptor(SubMeshDescriptor{5, 30, MeshTopology::Lines});
    ASSERT_EQ(m.getSubMeshCount(), 2);
}

TEST(Mesh, PushSubMeshDescriptorMakesGetSubMeshDescriptorReturnPushedDescriptor)
{
    Mesh m;
    SubMeshDescriptor const descriptor{0, 10, MeshTopology::Triangles};

    ASSERT_EQ(m.getSubMeshCount(), 0);
    m.pushSubMeshDescriptor(descriptor);
    ASSERT_EQ(m.getSubMeshDescriptor(0), descriptor);
}

TEST(Mesh, PushSecondDescriptorMakesGetReturnExpectedResults)
{
    Mesh m;
    SubMeshDescriptor const firstDesc{0, 10, MeshTopology::Triangles};
    SubMeshDescriptor const secondDesc{5, 15, MeshTopology::Lines};

    m.pushSubMeshDescriptor(firstDesc);
    m.pushSubMeshDescriptor(secondDesc);

    ASSERT_EQ(m.getSubMeshCount(), 2);
    ASSERT_EQ(m.getSubMeshDescriptor(0), firstDesc);
    ASSERT_EQ(m.getSubMeshDescriptor(1), secondDesc);
}

TEST(Mesh, GetSubMeshDescriptorThrowsOOBExceptionIfOOBAccessed)
{
    Mesh m;

    ASSERT_EQ(m.getSubMeshCount(), 0);
    ASSERT_ANY_THROW({ m.getSubMeshDescriptor(0); });

    m.pushSubMeshDescriptor({0, 10, MeshTopology::Triangles});
    ASSERT_EQ(m.getSubMeshCount(), 1);
    ASSERT_NO_THROW({ m.getSubMeshDescriptor(0); });
    ASSERT_ANY_THROW({ m.getSubMeshDescriptor(1); });
}

TEST(Mesh, ClearSubMeshDescriptorsDoesNothingOnEmptyMesh)
{
    Mesh m;
    ASSERT_NO_THROW({ m.clearSubMeshDescriptors(); });
}

TEST(Mesh, ClearSubMeshDescriptorsClearsAllDescriptors)
{
    Mesh m;
    m.pushSubMeshDescriptor({0, 10, MeshTopology::Triangles});
    m.pushSubMeshDescriptor({5, 15, MeshTopology::Lines});

    ASSERT_EQ(m.getSubMeshCount(), 2);
    ASSERT_NO_THROW({ m.clearSubMeshDescriptors(); });
    ASSERT_EQ(m.getSubMeshCount(), 0);
}

TEST(Mesh, GeneralClearMethodAlsoClearsSubMeshDescriptors)
{
    Mesh m;
    m.pushSubMeshDescriptor({0, 10, MeshTopology::Triangles});
    m.pushSubMeshDescriptor({5, 15, MeshTopology::Lines});

    ASSERT_EQ(m.getSubMeshCount(), 2);
    ASSERT_NO_THROW({ m.clear(); });
    ASSERT_EQ(m.getSubMeshCount(), 0);
}
