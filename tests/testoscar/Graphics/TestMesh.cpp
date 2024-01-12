#include <oscar/Graphics/Mesh.hpp>

#include <testoscar/TestingHelpers.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Graphics/SubMeshDescriptor.hpp>
#include <oscar/Graphics/VertexFormat.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Quat.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Triangle.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <oscar/Utils/Concepts.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <span>
#include <sstream>
#include <utility>
#include <vector>

using osc::testing::GenerateColors;
using osc::testing::GenerateColor32;
using osc::testing::GenerateIndices;
using osc::testing::GenerateNormals;
using osc::testing::GenerateTangents;
using osc::testing::GenerateTexCoords;
using osc::testing::GenerateTriangle;
using osc::testing::GenerateVec2;
using osc::testing::GenerateVec3;
using osc::testing::GenerateVec4;
using osc::testing::GenerateVertices;
using osc::testing::MapToVector;
using osc::testing::ResizedVectorCopy;
using osc::AABB;
using osc::AABBFromVerts;
using osc::BoundingAABBOf;
using osc::Color;
using osc::Color32;
using osc::Deg2Rad;
using osc::Identity;
using osc::Mat4;
using osc::Mesh;
using osc::MeshTopology;
using osc::MeshUpdateFlags;
using osc::Quat;
using osc::SubMeshDescriptor;
using osc::ToColor;
using osc::ToColor32;
using osc::ToMat4;
using osc::Transform;
using osc::TransformPoint;
using osc::Triangle;
using osc::VertexAttribute;
using osc::VertexAttributeFormat;
using osc::VertexFormat;
using osc::Vec2;
using osc::Vec3;
using osc::Vec4;

TEST(Mesh, CanBeDefaultConstructed)
{
    Mesh const mesh;
}

TEST(Mesh, CanBeCopyConstructed)
{
    Mesh const m;
    Mesh{m};
}

TEST(Mesh, CanBeMoveConstructed)
{
    Mesh m1;
    Mesh const m2{std::move(m1)};
}

TEST(Mesh, CanBeCopyAssigned)
{
    Mesh m1;
    Mesh const m2;

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
    Mesh const m;

    m.getTopology();
}

TEST(Mesh, GetTopologyDefaultsToTriangles)
{
    Mesh const m;

    ASSERT_EQ(m.getTopology(), MeshTopology::Triangles);
}

TEST(Mesh, SetTopologyCausesGetTopologyToUseSetValue)
{
    Mesh m;
    auto const newTopology = MeshTopology::Lines;

    ASSERT_NE(m.getTopology(), MeshTopology::Lines);

    m.setTopology(newTopology);

    ASSERT_EQ(m.getTopology(), newTopology);
}

TEST(Mesh, SetTopologyCausesCopiedMeshTobeNotEqualToInitialMesh)
{
    Mesh const m;
    Mesh copy{m};
    auto const newTopology = MeshTopology::Lines;

    ASSERT_EQ(m, copy);
    ASSERT_NE(copy.getTopology(), newTopology);

    copy.setTopology(newTopology);

    ASSERT_NE(m, copy);
}

TEST(Mesh, GetNumVertsInitiallyEmpty)
{
    ASSERT_EQ(Mesh{}.getNumVerts(), 0);
}

TEST(Mesh, Assigning3VertsMakesGetNumVertsReturn3Verts)
{
    Mesh m;
    m.setVerts(GenerateVertices(3));
    ASSERT_EQ(m.getNumVerts(), 3);
}

TEST(Mesh, HasVertsInitiallyfalse)
{
    ASSERT_FALSE(Mesh{}.hasVerts());
}

TEST(Mesh, HasVertsTrueAfterSettingVerts)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    ASSERT_TRUE(m.hasVerts());
}

TEST(Mesh, GetVertsReturnsEmptyVertsOnDefaultConstruction)
{
    ASSERT_TRUE(Mesh{}.getVerts().empty());
}

TEST(Mesh, SetVertsMakesGetCallReturnVerts)
{
    Mesh m;
    auto const verts = GenerateVertices(9);

    m.setVerts(verts);

    ASSERT_EQ(m.getVerts(), verts);
}

TEST(Mesh, SetVertsCausesCopiedMeshToNotBeEqualToInitialMesh)
{
    Mesh const m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.setVerts(GenerateVertices(30));

    ASSERT_NE(m, copy);
}

TEST(Mesh, ShrinkingVertsCausesNormalsToShrinkAlso)
{
    auto const normals = GenerateNormals(6);

    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setNormals(normals);
    m.setVerts(GenerateVertices(3));

    ASSERT_EQ(m.getNormals(), ResizedVectorCopy(normals, 3));
}

TEST(Mesh, ExpandingVertsCausesNormalsToExpandWithZeroedNormals)
{
    auto const normals = GenerateNormals(6);

    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setNormals(normals);
    m.setVerts(GenerateVertices(12));

    ASSERT_EQ(m.getNormals(), ResizedVectorCopy(normals, 12, Vec3{}));
}

TEST(Mesh, ShrinkingVertsCausesTexCoordsToShrinkAlso)
{
    auto uvs = GenerateTexCoords(6);

    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setTexCoords(uvs);
    m.setVerts(GenerateVertices(3));

    ASSERT_EQ(m.getTexCoords(), ResizedVectorCopy(uvs, 3));
}

TEST(Mesh, ExpandingVertsCausesTexCoordsToExpandWithZeroedTexCoords)
{
    auto const uvs = GenerateTexCoords(6);

    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setTexCoords(uvs);
    m.setVerts(GenerateVertices(12));

    ASSERT_EQ(m.getTexCoords(), ResizedVectorCopy(uvs, 12, Vec2{}));
}

TEST(Mesh, ShrinkingVertsCausesColorsToShrinkAlso)
{
    auto const colors = GenerateColors(6);

    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setColors(colors);
    m.setVerts(GenerateVertices(3));

    ASSERT_EQ(m.getColors(), ResizedVectorCopy(colors, 3));
}

TEST(Mesh, ExpandingVertsCausesColorsToExpandWithClearColor)
{
    auto const colors = GenerateColors(6);

    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setColors(colors);
    m.setVerts(GenerateVertices(12));

    ASSERT_EQ(m.getColors(), ResizedVectorCopy(colors, 12, Color::clear()));
}

TEST(Mesh, ShrinkingVertsCausesTangentsToShrinkAlso)
{
    auto const tangents = GenerateTangents(6);

    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setTangents(tangents);
    m.setVerts(GenerateVertices(3));

    auto const expected = ResizedVectorCopy(tangents, 3);
    auto const got = m.getTangents();
    ASSERT_EQ(got, expected);
}

TEST(Mesh, ExpandingVertsCausesTangentsToExpandAlsoAsZeroedTangents)
{
    auto const tangents = GenerateTangents(6);

    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setTangents(tangents);
    m.setVerts(GenerateVertices(12));  // resized

    auto const expected = ResizedVectorCopy(tangents, 12, Vec4{});
    auto const got = m.getTangents();
    ASSERT_EQ(got, expected);
}

TEST(Mesh, TransformVertsMakesGetCallReturnVerts)
{
    Mesh m;

    // generate "original" verts
    auto const originalVerts = GenerateVertices(30);

    // create "transformed" version of the verts
    auto const newVerts = MapToVector(originalVerts, [](auto const& v) { return v + 1.0f; });

    // sanity check that `setVerts` works as expected
    ASSERT_FALSE(m.hasVerts());
    m.setVerts(originalVerts);
    ASSERT_EQ(m.getVerts(), originalVerts);

    // the verts passed to `transformVerts` should match those returned by getVerts
    std::vector<Vec3> vertsPassedToTransformVerts;
    m.transformVerts([&vertsPassedToTransformVerts](Vec3& v) { vertsPassedToTransformVerts.push_back(v); });
    ASSERT_EQ(vertsPassedToTransformVerts, originalVerts);

    // applying the transformation should return the transformed verts
    m.transformVerts([&newVerts, i = 0](Vec3& v) mutable
    {
        v = newVerts.at(i++);
    });
    ASSERT_EQ(m.getVerts(), newVerts);
}

TEST(Mesh, TransformVertsCausesTransformedMeshToNotBeEqualToInitialMesh)
{
    Mesh const m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transformVerts([](Vec3&) {});  // noop transform also triggers this (meshes aren't value-comparable)

    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformVertsWithTransformAppliesTransformToVerts)
{
    // create appropriate transform
    Transform const transform = {
        .scale = Vec3{0.25f},
        .rotation = Quat{Vec3{Deg2Rad(90.0f), 0.0f, 0.0f}},
        .position = {1.0f, 0.25f, 0.125f},
    };

    // generate "original" verts
    auto const original = GenerateVertices(30);

    // precompute "expected" verts
    auto const expected = MapToVector(original, [&transform](auto const& p) { return TransformPoint(transform, p); });

    // create mesh with "original" verts
    Mesh m;
    m.setVerts(original);

    // then apply the transform
    m.transformVerts(transform);

    // the mesh's verts should match expectations
    ASSERT_EQ(m.getVerts(), expected);
}

TEST(Mesh, TransformVertsWithTransformCausesTransformedMeshToNotBeEqualToInitialMesh)
{
    Mesh const m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transformVerts(Identity<Transform>());  // noop transform also triggers this (meshes aren't value-comparable)

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
    auto const original = GenerateVertices(30);

    // precompute "expected" verts
    auto const expected = MapToVector(original, [&mat](auto const& p) { return TransformPoint(mat, p); });

    // create mesh with "original" verts
    Mesh m;
    m.setVerts(original);

    // then apply the transform
    m.transformVerts(mat);

    // the mesh's verts should match expectations
    ASSERT_EQ(m.getVerts(), expected);
}

TEST(Mesh, TransformVertsWithMat4CausesTransformedMeshToNotBeEqualToInitialMesh)
{
    Mesh const m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transformVerts(Identity<Mat4>());  // noop

    ASSERT_NE(m, copy) << "should be non-equal because mesh equality is reference-based (if it becomes value-based, delete this test)";
}

TEST(Mesh, HasNormalsReturnsFalseForNewlyConstructedMesh)
{
    ASSERT_FALSE(Mesh{}.hasNormals());
}

TEST(Mesh, AssigningOnlyNormalsButNoVertsMakesHasNormalsStillReturnFalse)
{
    Mesh m;
    m.setNormals(GenerateNormals(6));
    ASSERT_FALSE(m.hasNormals()) << "shouldn't have any normals, because the caller didn't first assign any vertices";
}

TEST(Mesh, AssigningNormalsAndThenVerticiesMakesNormalsAssignmentFail)
{
    Mesh m;
    m.setNormals(GenerateNormals(9));
    m.setVerts(GenerateVertices(9));
    ASSERT_FALSE(m.hasNormals()) << "shouldn't have any normals, because the caller assigned the vertices _after_ assigning the normals (must be first)";
}

TEST(Mesh, AssigningVerticesAndThenNormalsMakesHasNormalsReturnTrue)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setNormals(GenerateNormals(6));
    ASSERT_TRUE(m.hasNormals()) << "this should work: the caller assigned vertices (good) _and then_ normals (also good)";
}

TEST(Mesh, ClearingMeshClearsHasNormals)
{
    Mesh m;
    m.setVerts(GenerateVertices(3));
    m.setNormals(GenerateNormals(3));
    ASSERT_TRUE(m.hasNormals());
    m.clear();
    ASSERT_FALSE(m.hasNormals());
}

TEST(Mesh, HasNormalsReturnsFalseIfOnlyAssigningVerts)
{
    Mesh m;
    m.setVerts(GenerateVertices(3));
    ASSERT_FALSE(m.hasNormals()) << "shouldn't have normals: the caller didn't assign any vertices first";
}

TEST(Mesh, GetNormalsReturnsEmptyOnDefaultConstruction)
{
    Mesh m;
    ASSERT_TRUE(m.getNormals().empty());
}

TEST(Mesh, AssigningOnlyNormalsMakesGetNormalsReturnNothing)
{
    Mesh m;
    m.setNormals(GenerateNormals(3));

    ASSERT_TRUE(m.getNormals().empty()) << "should be empty, because the caller didn't first assign any vertices";
}

TEST(Mesh, AssigningNormalsAfterVerticesBehavesAsExpected)
{
    Mesh m;
    auto const normals = GenerateNormals(3);

    m.setVerts(GenerateVertices(3));
    m.setNormals(normals);

    ASSERT_EQ(m.getNormals(), normals) << "should assign the normals: the caller did what's expected";
}

TEST(Mesh, AssigningFewerNormalsThanVerticesShouldntAssignTheNormals)
{
    Mesh m;
    m.setVerts(GenerateVertices(9));
    m.setNormals(GenerateNormals(6));  // note: less than num verts
    ASSERT_FALSE(m.hasNormals()) << "normals were not assigned: different size from vertices";
}

TEST(Mesh, AssigningMoreNormalsThanVerticesShouldntAssignTheNormals)
{
    Mesh m;
    m.setVerts(GenerateVertices(9));
    m.setNormals(GenerateNormals(12));
    ASSERT_FALSE(m.hasNormals()) << "normals were not assigned: different size from vertices";
}

TEST(Mesh, SuccessfullyAsssigningNormalsChangesMeshEquality)
{
    Mesh m;
    m.setVerts(GenerateVertices(12));

    Mesh copy{m};
    ASSERT_EQ(m, copy);
    copy.setNormals(GenerateNormals(12));
    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformNormalsTransormsTheNormals)
{
    auto const transform = [](Vec3& n) { n = -n; };
    auto const original = GenerateNormals(16);
    auto expected = original;
    std::for_each(expected.begin(), expected.end(), transform);

    Mesh m;
    m.setVerts(GenerateVertices(16));
    m.setNormals(original);
    ASSERT_EQ(m.getNormals(), original);
    m.transformNormals(transform);

    auto const returned = m.getNormals();
    ASSERT_EQ(returned, expected);
}

TEST(Mesh, HasTexCoordsReturnsFalseForDefaultConstructedMesh)
{
    ASSERT_FALSE(Mesh{}.hasTexCoords());
}

TEST(Mesh, AssigningOnlyTexCoordsCausesHasTexCoordsToReturnFalse)
{
    Mesh m;
    m.setTexCoords(GenerateTexCoords(3));
    ASSERT_FALSE(m.hasTexCoords()) << "texture coordinates not assigned: no vertices";
}

TEST(Mesh, AssigningTexCoordsAndThenVerticesCausesHasTexCoordsToReturnFalse)
{
    Mesh m;
    m.setTexCoords(GenerateTexCoords(3));
    m.setVerts(GenerateVertices(3));
    ASSERT_FALSE(m.hasTexCoords()) << "texture coordinates not assigned: assigned in the wrong order";
}

TEST(Mesh, AssigningVerticesAndThenTexCoordsCausesHasTexCoordsToReturnTrue)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setTexCoords(GenerateTexCoords(6));
    ASSERT_TRUE(m.hasTexCoords());
}

TEST(Mesh, GetTexCoordsReturnsEmptyOnDefaultConstruction)
{
    Mesh m;
    ASSERT_TRUE(m.getTexCoords().empty());
}

TEST(Mesh, GetTexCoordsReturnsEmptyIfNoVerticesToAssignTheTexCoordsTo)
{
    Mesh m;
    m.setTexCoords(GenerateTexCoords(6));
    ASSERT_TRUE(m.getTexCoords().empty());
}

TEST(Mesh, GetTexCoordsReturnsSetCoordinatesWhenUsedNormally)
{
    Mesh m;
    m.setVerts(GenerateVertices(12));
    auto const coords = GenerateTexCoords(12);
    m.setTexCoords(coords);
    ASSERT_EQ(m.getTexCoords(), coords);
}

TEST(Mesh, SetTexCoordsDoesNotSetCoordsIfGivenLessCoordsThanVerts)
{
    Mesh m;
    m.setVerts(GenerateVertices(12));
    m.setTexCoords(GenerateTexCoords(9));  // note: less
    ASSERT_FALSE(m.hasTexCoords());
    ASSERT_TRUE(m.getTexCoords().empty());
}

TEST(Mesh, SetTexCoordsDoesNotSetCoordsIfGivenMoreCoordsThanVerts)
{
    Mesh m;
    m.setVerts(GenerateVertices(12));
    m.setTexCoords(GenerateTexCoords(15));  // note: more
    ASSERT_FALSE(m.hasTexCoords());
    ASSERT_TRUE(m.getTexCoords().empty());
}

TEST(Mesh, SuccessfulSetCoordsCausesCopiedMeshToBeNotEqualToOriginalMesh)
{
    Mesh m;
    m.setVerts(GenerateVertices(12));
    Mesh copy{m};
    ASSERT_EQ(m, copy);
    copy.setTexCoords(GenerateTexCoords(12));
    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformTexCoordsAppliesTransformToTexCoords)
{
    auto const transform = [](Vec2& uv) { uv *= 0.287f; };
    auto const original = GenerateTexCoords(3);
    auto expected = original;
    std::for_each(expected.begin(), expected.end(), transform);

    Mesh m;
    m.setVerts(GenerateVertices(3));
    m.setTexCoords(original);
    ASSERT_EQ(m.getTexCoords(), original);
    m.transformTexCoords(transform);
    ASSERT_EQ(m.getTexCoords(), expected);
}

TEST(Mesh, GetColorsInitiallyReturnsEmptySpan)
{
    ASSERT_TRUE(Mesh{}.getColors().empty());
}

TEST(Mesh, GetColorsRemainsEmptyIfAssignedWithNoVerts)
{
    Mesh m;
    ASSERT_TRUE(m.getColors().empty());
    m.setColors(GenerateColors(3));
    ASSERT_TRUE(m.getColors().empty()) << "no verticies to assign colors to";
}

TEST(Mesh, GetColorsReturnsSetColorsWhenAssignedToVertices)
{
    Mesh m;
    m.setVerts(GenerateVertices(9));
    auto const colors = GenerateColors(9);
    m.setColors(colors);
    ASSERT_FALSE(m.getColors().empty());
    ASSERT_EQ(m.getColors(), colors);
}

TEST(Mesh, SetColorsAssignmentFailsIfGivenFewerColorsThanVerts)
{
    Mesh m;
    m.setVerts(GenerateVertices(9));
    m.setColors(GenerateColors(6));  // note: less
    ASSERT_TRUE(m.getColors().empty());
}

TEST(Mesh, SetColorsAssignmentFailsIfGivenMoreColorsThanVerts)
{
    Mesh m;
    m.setVerts(GenerateVertices(9));
    m.setColors(GenerateColors(12));  // note: more
    ASSERT_TRUE(m.getColors().empty());
}

TEST(Mesh, GetTangentsInitiallyReturnsEmptySpan)
{
    Mesh m;
    ASSERT_TRUE(m.getTangents().empty());
}

TEST(Mesh, SetTangentsFailsWhenAssigningWithNoVerts)
{
    Mesh m;
    m.setTangents(GenerateTangents(3));
    ASSERT_TRUE(m.getTangents().empty());
}

TEST(Mesh, SetTangentsWorksWhenAssigningToCorrectNumberOfVertices)
{
    Mesh m;
    m.setVerts(GenerateVertices(15));
    auto const tangents = GenerateTangents(15);
    m.setTangents(tangents);
    ASSERT_FALSE(m.getTangents().empty());
    ASSERT_EQ(m.getTangents(), tangents);
}

TEST(Mesh, SetTangentsFailsIfFewerTangentsThanVerts)
{
    Mesh m;
    m.setVerts(GenerateVertices(15));
    m.setTangents(GenerateTangents(12));  // note: fewer
    ASSERT_TRUE(m.getTangents().empty());
}

TEST(Mesh, SetTangentsFailsIfMoreTangentsThanVerts)
{
    Mesh m;
    m.setVerts(GenerateVertices(15));
    m.setTangents(GenerateTangents(18));  // note: more
    ASSERT_TRUE(m.getTangents().empty());
}

TEST(Mesh, GetNumIndicesReturnsZeroOnDefaultConstruction)
{
    Mesh m;
    ASSERT_EQ(m.getNumIndices(), 0);
}

TEST(Mesh, GetNumIndicesReturnsNumberOfAssignedIndices)
{
    auto const verts = GenerateVertices(3);
    auto const indices = GenerateIndices(0, 3);

    Mesh m;
    m.setVerts(verts);
    m.setIndices(indices);

    ASSERT_EQ(m.getNumIndices(), 3);
}

TEST(Mesh, SetIndiciesWithNoFlagsWorksForNormalArgs)
{
    auto const indices = GenerateIndices(0, 3);

    Mesh m;
    m.setVerts(GenerateVertices(3));
    m.setIndices(indices);

    ASSERT_EQ(m.getNumIndices(), 3);
}

TEST(Mesh, SetIndicesAlsoWorksIfOnlyIndexesSomeOfTheVerts)
{
    auto const indices = GenerateIndices(3, 6);  // only indexes half the verts

    Mesh m;
    m.setVerts(GenerateVertices(6));
    ASSERT_NO_THROW({ m.setIndices(indices); });
}

TEST(Mesh, SetIndicesThrowsIfOutOfBounds)
{
    Mesh m;
    m.setVerts(GenerateVertices(3));
    ASSERT_ANY_THROW({ m.setIndices(GenerateIndices(3, 6)); }) << "should throw: indices are out-of-bounds";
}

TEST(Mesh, SetIndiciesWithDontValidateIndicesAndDontRecalculateBounds)
{
    Mesh m;
    m.setVerts(GenerateVertices(3));
    ASSERT_NO_THROW({ m.setIndices(GenerateIndices(3, 6), MeshUpdateFlags::DontValidateIndices | MeshUpdateFlags::DontRecalculateBounds); }) << "shouldn't throw: we explicitly asked the engine to not check indices";
}

TEST(Mesh, SetIndicesRecalculatesBounds)
{
    Triangle const triangle = GenerateTriangle();

    Mesh m;
    m.setVerts(triangle);
    ASSERT_EQ(m.getBounds(), AABB{});
    m.setIndices(GenerateIndices(0, 3));
    ASSERT_EQ(m.getBounds(), BoundingAABBOf(triangle));
}

TEST(Mesh, SetIndicesWithDontRecalculateBoundsDoesNotRecalculateBounds)
{
    Triangle const triangle = GenerateTriangle();

    Mesh m;
    m.setVerts(triangle);
    ASSERT_EQ(m.getBounds(), AABB{});
    m.setIndices(GenerateIndices(0, 3), MeshUpdateFlags::DontRecalculateBounds);
    ASSERT_EQ(m.getBounds(), AABB{}) << "bounds shouldn't update: we explicitly asked for the engine to skip it";
}

TEST(Mesh, ForEachIndexedVertNotCalledWithEmptyMesh)
{
    size_t ncalls = 0;
    Mesh{}.forEachIndexedVert([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, ForEachIndexedVertNotCalledWhenOnlyVertexDataSupplied)
{
    Mesh m;
    m.setVerts({{Vec3{}, Vec3{}, Vec3{}}});
    size_t ncalls = 0;
    m.forEachIndexedVert([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, ForEachIndexedVertCalledWhenSuppliedCorrectlyIndexedMesh)
{
    Mesh m;
    m.setVerts({{Vec3{}, Vec3{}, Vec3{}}});
    m.setIndices(std::to_array<uint16_t>({0, 1, 2}));
    size_t ncalls = 0;
    m.forEachIndexedVert([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 3);
}

TEST(Mesh, ForEachIndexedVertCalledEvenWhenMeshIsNonTriangular)
{
    Mesh m;
    m.setTopology(MeshTopology::Lines);
    m.setVerts({{Vec3{}, Vec3{}, Vec3{}, Vec3{}}});
    m.setIndices(std::to_array<uint16_t>({0, 1, 2, 3}));
    size_t ncalls = 0;
    m.forEachIndexedVert([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 4);
}

TEST(Mesh, ForEachIndexedTriangleNotCalledWithEmptyMesh)
{
    size_t ncalls = 0;
    Mesh{}.forEachIndexedTriangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, ForEachIndexedTriangleNotCalledWhenMeshhasNoIndicies)
{
    Mesh m;
    m.setVerts({{Vec3{}, Vec3{}, Vec3{}}});  // unindexed
    size_t ncalls = 0;
    m.forEachIndexedTriangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, ForEachIndexedTriangleCalledIfMeshContainsIndexedTriangles)
{
    Mesh m;
    m.setVerts({{Vec3{}, Vec3{}, Vec3{}}});
    m.setIndices(std::to_array<uint16_t>({0, 1, 2}));
    size_t ncalls = 0;
    m.forEachIndexedTriangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 1);
}

TEST(Mesh, ForEachIndexedTriangleNotCalledIfMeshContainsInsufficientIndices)
{
    Mesh m;
    m.setVerts({{Vec3{}, Vec3{}, Vec3{}}});
    m.setIndices(std::to_array<uint16_t>({0, 1}));  // too few
    size_t ncalls = 0;
    m.forEachIndexedTriangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, ForEachIndexedTriangleCalledMultipleTimesForMultipleTriangles)
{
    Mesh m;
    m.setVerts({{Vec3{}, Vec3{}, Vec3{}}});
    m.setIndices(std::to_array<uint16_t>({0, 1, 2, 1, 2, 0}));
    size_t ncalls = 0;
    m.forEachIndexedTriangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 2);
}

TEST(Mesh, ForEachIndexedTriangleNotCalledIfMeshTopologyIsLines)
{
    Mesh m;
    m.setTopology(MeshTopology::Lines);
    m.setVerts({{Vec3{}, Vec3{}, Vec3{}}});
    m.setIndices(std::to_array<uint16_t>({0, 1, 2, 1, 2, 0}));
    size_t ncalls = 0;
    m.forEachIndexedTriangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, GetTriangleAtReturnsExpectedTriangleForNormalCase)
{
    Triangle const t = GenerateTriangle();

    Mesh m;
    m.setVerts(t);
    m.setIndices(std::to_array<uint16_t>({0, 1, 2}));

    ASSERT_EQ(m.getTriangleAt(0), t);
}

TEST(Mesh, GetTriangleAtReturnsTriangleIndexedByIndiciesAtProvidedOffset)
{
    Triangle const a = GenerateTriangle();
    Triangle const b = GenerateTriangle();

    Mesh m;
    m.setVerts({{a[0], a[1], a[2], b[0], b[1], b[2]}});         // stored as  [a, b]
    m.setIndices(std::to_array<uint16_t>({3, 4, 5, 0, 1, 2}));  // indexed as [b, a]

    ASSERT_EQ(m.getTriangleAt(0), b) << "the provided arg is an offset into the _indices_";
    ASSERT_EQ(m.getTriangleAt(3), a) << "the provided arg is an offset into the _indices_";
}

TEST(Mesh, GetTriangleAtThrowsIfCalledOnNonTriangularMesh)
{
    Mesh m;
    m.setTopology(MeshTopology::Lines);
    m.setVerts({{GenerateVec3(), GenerateVec3(), GenerateVec3(), GenerateVec3(), GenerateVec3(), GenerateVec3()}});
    m.setIndices(std::to_array<uint16_t>({0, 1, 2, 3, 4, 5}));

    ASSERT_ANY_THROW({ m.getTriangleAt(0); }) << "incorrect topology";
}

TEST(Mesh, GetTriangleAtThrowsIfGivenOutOfBoundsIndexOffset)
{
    Triangle const t = GenerateTriangle();

    Mesh m;
    m.setVerts(t);
    m.setIndices(std::to_array<uint16_t>({0, 1, 2}));

    ASSERT_ANY_THROW({ m.getTriangleAt(1); }) << "should throw: it's out-of-bounds";
    ASSERT_ANY_THROW({ m.getTriangleAt(2); }) << "should throw: it's out-of-bounds";
    ASSERT_ANY_THROW({ m.getTriangleAt(3); }) << "should throw: it's out-of-bounds";
}

TEST(Mesh, GetIndexedVertsReturnsEmptyArrayForBlankMesh)
{
    ASSERT_TRUE(Mesh{}.getIndexedVerts().empty());
}

TEST(Mesh, GetIndexedVertsReturnsEmptyArrayForMeshWithVertsButNoIndices)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));

    ASSERT_TRUE(m.getIndexedVerts().empty());
}

TEST(Mesh, GetIndexedVertsReturnsOnlyTheIndexedVerts)
{
    auto const allVerts = GenerateVertices(12);
    auto const subIndices = GenerateIndices(5, 8);

    Mesh m;
    m.setVerts(allVerts);
    m.setIndices(subIndices);

    auto const expected = MapToVector(std::span{allVerts}.subspan(5, 3), std::identity{});
    auto const got = m.getIndexedVerts();

    ASSERT_EQ(m.getIndexedVerts(), expected);
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
    ASSERT_EQ(m.getBounds(), AABBFromVerts(pyramid));
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

TEST(Mesh, GetVertexAttributeCountInitiallyZero)
{
    ASSERT_EQ(Mesh{}.getVertexAttributeCount(), 0);
}

TEST(Mesh, GetVertexAttributeCountBecomes1AfterSettingVerts)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 1);
}

TEST(Mesh, GetVertexAttributeCountRezeroesIfVerticesAreCleared)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 1);
    m.setVerts({});
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
}

TEST(Mesh, GetVertexAttributeCountGoesToTwoAfterSetting)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setNormals(GenerateNormals(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
}

TEST(Mesh, GetVertexAttributeCountGoesDownAsExpectedWrtNormals)
{
    Mesh m;
    m.setVerts(GenerateVertices(12));
    m.setNormals(GenerateNormals(12));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.setNormals({});  // clear normals: should only clear the normals
    ASSERT_EQ(m.getVertexAttributeCount(), 1);
    m.setNormals(GenerateNormals(12));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.setVerts({});  // clear verts: should clear vertices + attributes (here: normals)
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
}

TEST(Mesh, GetVertexAttributeCountIsZeroAfterClearingWholeMeshWrtNormals)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setNormals(GenerateNormals(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.clear();
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
}

TEST(Mesh, GetVertexAttributeCountGoesToTwoAfterAssigningTexCoords)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setTexCoords(GenerateTexCoords(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
}

TEST(Mesh, GetVertexAttributeCountGoesBackToOneAfterClearingTexCoords)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setTexCoords(GenerateTexCoords(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.setTexCoords({}); // clear them
    ASSERT_EQ(m.getVertexAttributeCount(), 1);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedWrtTexCoords)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setTexCoords(GenerateTexCoords(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.setVerts({});
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
    m.setVerts(GenerateVertices(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 1);
    m.setTexCoords(GenerateTexCoords(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.clear();
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedWrtColors)
{
    Mesh m;
    m.setVerts(GenerateVertices(12));
    m.setColors(GenerateColors(12));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.setColors({});
    ASSERT_EQ(m.getVertexAttributeCount(), 1);
    m.setColors(GenerateColors(12));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.setVerts({});
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
    m.setVerts(GenerateVertices(12));
    m.setColors(GenerateColors(12));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.clear();
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedWrtTangents)
{
    Mesh m;
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
    m.setVerts(GenerateVertices(9));
    ASSERT_EQ(m.getVertexAttributeCount(), 1);
    m.setTangents(GenerateTangents(9));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.setTangents({});
    ASSERT_EQ(m.getVertexAttributeCount(), 1);
    m.setTangents(GenerateTangents(9));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.setVerts({});
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
    m.setVerts(GenerateVertices(9));
    m.setTangents(GenerateTangents(9));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.clear();
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedForMultipleAttributes)
{
    Mesh m;

    // first, try adding all possible attributes
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
    m.setVerts(GenerateVertices(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 1);
    m.setNormals(GenerateNormals(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    m.setTexCoords(GenerateTexCoords(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 3);
    m.setColors(GenerateColors(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 4);
    m.setTangents(GenerateTangents(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 5);

    // then make sure that assigning over them doesn't change
    // the number of attributes (i.e. it's an in-place assignment)
    ASSERT_EQ(m.getVertexAttributeCount(), 5);
    m.setVerts(GenerateVertices(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 5);
    m.setNormals(GenerateNormals(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 5);
    m.setTexCoords(GenerateTexCoords(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 5);
    m.setColors(GenerateColors(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 5);
    m.setTangents(GenerateTangents(6));
    ASSERT_EQ(m.getVertexAttributeCount(), 5);

    // then make sure that attributes can be deleted in a different
    // order from assignment, and attribute count behaves as-expected
    {
        Mesh copy = m;
        ASSERT_EQ(copy.getVertexAttributeCount(), 5);
        copy.setTexCoords({});
        ASSERT_EQ(copy.getVertexAttributeCount(), 4);
        copy.setColors({});
        ASSERT_EQ(copy.getVertexAttributeCount(), 3);
        copy.setNormals({});
        ASSERT_EQ(copy.getVertexAttributeCount(), 2);
        copy.setTangents({});
        ASSERT_EQ(copy.getVertexAttributeCount(), 1);
        copy.setVerts({});
        ASSERT_EQ(copy.getVertexAttributeCount(), 0);
    }

    // ... and Mesh::clear behaves as expected
    {
        Mesh copy = m;
        ASSERT_EQ(copy.getVertexAttributeCount(), 5);
        copy.clear();
        ASSERT_EQ(copy.getVertexAttributeCount(), 0);
    }

    // ... and clearing the verts first clears all attributes
    {
        Mesh copy = m;
        ASSERT_EQ(copy.getVertexAttributeCount(), 5);
        copy.setVerts({});
        ASSERT_EQ(copy.getVertexAttributeCount(), 0);
    }
}

TEST(Mesh, GetVertexAttributesReturnsEmptyOnConstruction)
{
    ASSERT_TRUE(Mesh{}.getVertexAttributes().empty());
}

TEST(Mesh, GetVertexAttributesReturnsExpectedAttributeWhenJustVerticesAreSet)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));

    VertexFormat const expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    };

    ASSERT_EQ(m.getVertexAttributes(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedAttributesWhenVerticesAndNormalsSet)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setNormals(GenerateNormals(6));

    VertexFormat const expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,   VertexAttributeFormat::Float32x3},
    };

    ASSERT_EQ(m.getVertexAttributes(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedWhenVerticesAndTexCoordsSet)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setTexCoords(GenerateTexCoords(6));

    VertexFormat const expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_EQ(m.getVertexAttributes(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedWhenVerticesAndColorsSet)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setColors(GenerateColors(6));

    VertexFormat const expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Color,    VertexAttributeFormat::Float32x4},
    };

    ASSERT_EQ(m.getVertexAttributes(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedWhenVerticesAndTangentsSet)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setTangents(GenerateTangents(6));

    VertexFormat const expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent,  VertexAttributeFormat::Float32x4},
    };

    ASSERT_EQ(m.getVertexAttributes(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedForCombinations)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setNormals(GenerateNormals(6));
    m.setTexCoords(GenerateTexCoords(6));

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
        };
        ASSERT_EQ(m.getVertexAttributes(), expected);
    }

    m.setColors(GenerateColors(6));

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
        };
        ASSERT_EQ(m.getVertexAttributes(), expected);
    }

    m.setTangents(GenerateTangents(6));

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
        };
        ASSERT_EQ(m.getVertexAttributes(), expected);
    }

    m.setColors({});  // clear color

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
        };
        ASSERT_EQ(m.getVertexAttributes(), expected);
    }

    m.setColors(GenerateColors(6));

    // check that ordering is based on when it was set
    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
        };
        ASSERT_EQ(m.getVertexAttributes(), expected);
    }

    m.setNormals({});

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
        };
        ASSERT_EQ(m.getVertexAttributes(), expected);
    }

    Mesh copy{m};

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
        };
        ASSERT_EQ(copy.getVertexAttributes(), expected);
    }

    m.setVerts({});

    {
        VertexFormat const expected;
        ASSERT_EQ(m.getVertexAttributes(), expected);
        ASSERT_NE(copy.getVertexAttributes(), expected) << "the copy should be independent";
    }

    copy.clear();

    {
        VertexFormat const expected;
        ASSERT_EQ(copy.getVertexAttributes(), expected);
    }
}

TEST(Mesh, SetVertexBufferParamsWithEmptyDescriptorIgnoresN)
{
    Mesh m;
    m.setVerts(GenerateVertices(9));

    ASSERT_EQ(m.getNumVerts(), 9);
    ASSERT_EQ(m.getVertexAttributeCount(), 1);

    m.setVertexBufferParams(15, {});  // i.e. no data, incl. positions

    ASSERT_EQ(m.getNumVerts(), 0);  // i.e. the 15 was effectively ignored, because there's no attributes
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
}

TEST(Mesh, SetVertexBufferParamsWithEmptyDescriptorClearsAllAttributesNotJustPosition)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setNormals(GenerateNormals(6));
    m.setColors(GenerateColors(6));

    ASSERT_EQ(m.getNumVerts(), 6);
    ASSERT_EQ(m.getVertexAttributeCount(), 3);

    m.setVertexBufferParams(24, {});

    ASSERT_EQ(m.getNumVerts(), 0);
    ASSERT_EQ(m.getVertexAttributeCount(), 0);
}

TEST(Mesh, SetVertexBufferParamsExpandsPositionsWithZeroedVectors)
{
    auto const verts = GenerateVertices(6);

    Mesh m;
    m.setVerts(verts);
    m.setVertexBufferParams(12, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3}
    });

    auto expected = verts;
    expected.resize(12, Vec3{});

    ASSERT_EQ(m.getVerts(), expected);
}

TEST(Mesh, SetVertexBufferParamsCanShrinkPositionVectors)
{
    auto const verts = GenerateVertices(12);

    Mesh m;
    m.setVerts(verts);
    m.setVertexBufferParams(6, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3}
    });

    auto expected = verts;
    expected.resize(6);

    ASSERT_EQ(m.getVerts(), expected);
}

TEST(Mesh, SetVertexBufferParamsWhenDimensionalityOfVerticesIs2ZeroesTheMissingDimension)
{
    auto const verts = GenerateVertices(6);

    Mesh m;
    m.setVerts(verts);
    m.setVertexBufferParams(6, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},  // 2D storage
    });

    auto const expected = MapToVector(verts, [](Vec3 const& v) { return Vec3{v.x, v.y, 0.0f}; });

    ASSERT_EQ(m.getVerts(), expected);
}

TEST(Mesh, SetVertexBufferParamsCanBeUsedToRemoveAParticularAttribute)
{
    auto const verts = GenerateVertices(6);
    auto const tangents = GenerateTangents(6);

    Mesh m;
    m.setVerts(verts);
    m.setNormals(GenerateNormals(6));
    m.setTangents(tangents);

    ASSERT_EQ(m.getVertexAttributeCount(), 3);

    m.setVertexBufferParams(6, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent,  VertexAttributeFormat::Float32x4},
        // i.e. remove the normals
    });

    ASSERT_EQ(m.getNumVerts(), 6);
    ASSERT_EQ(m.getVertexAttributeCount(), 2);
    ASSERT_EQ(m.getVerts(), verts);
    ASSERT_EQ(m.getTangents(), tangents);
}

TEST(Mesh, SetVertexBufferParamsCanBeUsedToAddAParticularAttributeAsZeroed)
{
    auto const verts = GenerateVertices(6);
    auto const tangents = GenerateTangents(6);

    Mesh m;
    m.setVerts(verts);
    m.setTangents(tangents);

    ASSERT_EQ(m.getVertexAttributeCount(), 2);

    m.setVertexBufferParams(6, {
        // existing
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
        // new
        {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    });

    ASSERT_EQ(m.getVerts(), verts);
    ASSERT_EQ(m.getTangents(), tangents);
    ASSERT_EQ(m.getColors(), std::vector<Color>(6));
    ASSERT_EQ(m.getTexCoords(), std::vector<Vec2>(6));
}

TEST(Mesh, SetVertexBufferParamsThrowsIfItCausesIndicesToGoOutOfBounds)
{
    Mesh m;
    m.setVerts(GenerateVertices(6));
    m.setIndices(GenerateIndices(0, 6));

    ASSERT_ANY_THROW({ m.setVertexBufferParams(3, m.getVertexAttributes()); })  << "should throw because indices are now OOB";
}

TEST(Mesh, SetVertexBufferParamsCanBeUsedToReformatToU8NormFormat)
{
    auto const colors = GenerateColors(9);

    Mesh m;
    m.setVerts(GenerateVertices(9));
    m.setColors(colors);

    ASSERT_EQ(m.getColors(), colors);

    m.setVertexBufferParams(9, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Color,    VertexAttributeFormat::Unorm8x4},
    });

    auto const expected = MapToVector(colors, [](Color const& c) {
        return ToColor(ToColor32(c));
    });

    ASSERT_EQ(m.getColors(), expected);
}

TEST(Mesh, GetVertexBufferStrideReturnsExpectedResults)
{
    Mesh m;
    ASSERT_EQ(m.getVertexBufferStride(), 0);

    m.setVertexBufferParams(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    });
    ASSERT_EQ(m.getVertexBufferStride(), 3*sizeof(float));

    m.setVertexBufferParams(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},
    });
    ASSERT_EQ(m.getVertexBufferStride(), 2*sizeof(float));

    m.setVertexBufferParams(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},
        {VertexAttribute::Color,    VertexAttributeFormat::Float32x4},
    });
    ASSERT_EQ(m.getVertexBufferStride(), 2*sizeof(float)+4*sizeof(float));

    m.setVertexBufferParams(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},
        {VertexAttribute::Color,    VertexAttributeFormat::Unorm8x4},
    });
    ASSERT_EQ(m.getVertexBufferStride(), 2*sizeof(float)+4);

    m.setVertexBufferParams(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::Color,    VertexAttributeFormat::Unorm8x4},
    });
    ASSERT_EQ(m.getVertexBufferStride(), 4+4);

    m.setVertexBufferParams(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},
        {VertexAttribute::Tangent,  VertexAttributeFormat::Float32x4},
        {VertexAttribute::Color,    VertexAttributeFormat::Unorm8x4},
    });
    ASSERT_EQ(m.getVertexBufferStride(), 2*sizeof(float) + 4 + 4*sizeof(float));
}

TEST(Mesh, SetVertexBufferDataWorksForSimplestCase)
{
    struct Entry final {
        Vec3 vert = GenerateVec3();
    };
    std::vector<Entry> const data(12);

    Mesh m;
    m.setVertexBufferParams(12, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    });
    m.setVertexBufferData(data);

    auto const expected = MapToVector(data, [](auto const& entry) { return entry.vert; });

    ASSERT_EQ(m.getVerts(), expected);
}

TEST(Mesh, SetVertexBufferDataFailsInSimpleCaseIfAttributeMismatches)
{
    struct Entry final {
        Vec3 vert = GenerateVec3();
    };
    std::vector<Entry> const data(12);

    Mesh m;
    m.setVertexBufferParams(12, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},  // uh oh: wrong dimensionality for `Entry`
    });
    ASSERT_ANY_THROW({ m.setVertexBufferData(data); });
}

TEST(Mesh, SetVertexBufferDataFailsInSimpleCaseIfNMismatches)
{
    struct Entry final {
        Vec3 vert = GenerateVec3();
    };
    std::vector<Entry> const data(12);

    Mesh m;
    m.setVertexBufferParams(6, {  // uh oh: wrong N for the given number of entries
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    });
    ASSERT_ANY_THROW({ m.setVertexBufferData(data); });
}

TEST(Mesh, SetVertexBufferDataDoesntFailIfTheCallerLuckilyProducesSameLayout)
{
    struct Entry final {
        Vec4 vert = GenerateVec4();  // note: Vec4
    };
    std::vector<Entry> const data(12);

    Mesh m;
    m.setVertexBufferParams(24, {  // uh oh
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},  // ah, but, the total size will now luckily match...
    });
    ASSERT_NO_THROW({ m.setVertexBufferData(data); });  // and it won't throw because the API cannot know any better...
}

TEST(Mesh, SetVertexBufferDataThrowsIfLayoutNotProvided)
{
    struct Entry final {
        Vec3 verts;
    };
    std::vector<Entry> const data(12);

    Mesh m;
    ASSERT_ANY_THROW({ m.setVertexBufferData(data); }) << "should throw: caller didn't call 'setVertexBufferParams' first";
}

TEST(Mesh, SetVertexBufferDataWorksAsExpectedForImguiStyleCase)
{
    struct SimilarToImGuiVert final {
        Vec2 pos = GenerateVec2();
        Color32 col = GenerateColor32();
        Vec2 uv = GenerateVec2();
    };
    std::vector<SimilarToImGuiVert> const data(16);
    auto const expectedVerts = MapToVector(data, [](auto const& v) { return Vec3{v.pos, 0.0f}; });
    auto const expectedColors = MapToVector(data, [](auto const& v) { return ToColor(v.col); });
    auto const expectedTexCoords = MapToVector(data, [](auto const& v) { return v.uv; });

    Mesh m;
    m.setVertexBufferParams(16, {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x2},
        {VertexAttribute::Color,     VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    });

    // directly set vertex buffer data
    ASSERT_EQ(m.getVertexBufferStride(), sizeof(SimilarToImGuiVert));
    ASSERT_NO_THROW({ m.setVertexBufferData(data); });

    auto const verts = m.getVerts();
    auto const colors = m.getColors();
    auto const texCoords = m.getTexCoords();

    ASSERT_EQ(verts, expectedVerts);
    ASSERT_EQ(colors, expectedColors);
    ASSERT_EQ(texCoords, expectedTexCoords);
}

TEST(Mesh, SetVertexBufferDataRecalculatesBounds)
{
    auto firstVerts = GenerateVertices(6);
    auto secondVerts = MapToVector(firstVerts, [](auto const& v) { return 2.0f*v; });  // i.e. has different bounds

    Mesh m;
    m.setVerts(firstVerts);
    m.setIndices(GenerateIndices(0, 6));

    ASSERT_EQ(m.getBounds(), BoundingAABBOf(firstVerts));

    m.setVertexBufferData(secondVerts);

    ASSERT_EQ(m.getBounds(), BoundingAABBOf(secondVerts));
}
