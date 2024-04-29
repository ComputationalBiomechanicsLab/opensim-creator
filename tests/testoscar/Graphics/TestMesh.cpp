#include <oscar/Graphics/Mesh.h>

#include <testoscar/TestingHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/SubMeshDescriptor.h>
#include <oscar/Graphics/VertexFormat.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Eulers.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/TriangleFunctions.h>
#include <oscar/Maths/UnitVec3.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

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

using namespace osc;
using namespace osc::literals;
using namespace osc::testing;

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

    m.topology();
}

TEST(Mesh, GetTopologyDefaultsToTriangles)
{
    Mesh const m;

    ASSERT_EQ(m.topology(), MeshTopology::Triangles);
}

TEST(Mesh, SetTopologyCausesGetTopologyToUseSetValue)
{
    Mesh m;
    auto const newTopology = MeshTopology::Lines;

    ASSERT_NE(m.topology(), MeshTopology::Lines);

    m.set_topology(newTopology);

    ASSERT_EQ(m.topology(), newTopology);
}

TEST(Mesh, SetTopologyCausesCopiedMeshTobeNotEqualToInitialMesh)
{
    Mesh const m;
    Mesh copy{m};
    auto const newTopology = MeshTopology::Lines;

    ASSERT_EQ(m, copy);
    ASSERT_NE(copy.topology(), newTopology);

    copy.set_topology(newTopology);

    ASSERT_NE(m, copy);
}

TEST(Mesh, GetNumVertsInitiallyEmpty)
{
    ASSERT_EQ(Mesh{}.num_vertices(), 0);
}

TEST(Mesh, Assigning3VertsMakesGetNumVertsReturn3Verts)
{
    Mesh m;
    m.set_vertices(GenerateVertices(3));
    ASSERT_EQ(m.num_vertices(), 3);
}

TEST(Mesh, HasVertsInitiallyfalse)
{
    ASSERT_FALSE(Mesh{}.has_vertices());
}

TEST(Mesh, HasVertsTrueAfterSettingVerts)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    ASSERT_TRUE(m.has_vertices());
}

TEST(Mesh, GetVertsReturnsEmptyVertsOnDefaultConstruction)
{
    ASSERT_TRUE(Mesh{}.vertices().empty());
}

TEST(Mesh, SetVertsMakesGetCallReturnVerts)
{
    Mesh m;
    auto const verts = GenerateVertices(9);

    m.set_vertices(verts);

    ASSERT_EQ(m.vertices(), verts);
}

TEST(Mesh, SetVertsCanBeCalledWithInitializerList)
{
    Mesh m;

    Vec3 a{};
    Vec3 b{};
    Vec3 c{};

    m.set_vertices({a, b, c});
    std::vector<Vec3> expected = {a, b, c};

    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, SetVertsCanBeCalledWithUnitVectorsBecauseOfImplicitConversion)
{
    Mesh m;
    UnitVec3 v{1.0f, 0.0f, 0.0f};
    m.set_vertices({v});
    std::vector<Vec3> expected = {v};
    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, SetVertsCausesCopiedMeshToNotBeEqualToInitialMesh)
{
    Mesh const m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.set_vertices(GenerateVertices(30));

    ASSERT_NE(m, copy);
}

TEST(Mesh, ShrinkingVertsCausesNormalsToShrinkAlso)
{
    auto const normals = GenerateNormals(6);

    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_normals(normals);
    m.set_vertices(GenerateVertices(3));

    ASSERT_EQ(m.normals(), ResizedVectorCopy(normals, 3));
}

TEST(Mesh, CanCallSetNormalsWithInitializerList)
{
    auto const verts = GenerateVertices(3);
    auto const normals = GenerateNormals(3);

    Mesh m;
    m.set_vertices(verts);
    m.set_normals({normals[0], normals[1], normals[2]});

    ASSERT_EQ(m.normals(), normals);
}

TEST(Mesh, CanCallSetTexCoordsWithInitializerList)
{
    auto const verts = GenerateVertices(3);
    auto const uvs = GenerateTexCoords(3);

    Mesh m;
    m.set_vertices(verts);
    m.set_tex_coords({uvs[0], uvs[1], uvs[2]});

    ASSERT_EQ(m.tex_coords(), uvs);
}

TEST(Mesh, CanCallSetColorsWithInitializerList)
{
    auto const verts = GenerateVertices(3);
    auto const colors = GenerateColors(3);

    Mesh m;
    m.set_vertices(verts);
    m.set_colors({colors[0], colors[1], colors[2]});

    ASSERT_EQ(m.colors(), colors);
}

TEST(Mesh, CanCallSetTangentsWithInitializerList)
{
    auto const verts = GenerateVertices(3);
    auto const tangents = GenerateTangents(3);

    Mesh m;
    m.set_vertices(verts);
    m.set_tangents({tangents[0], tangents[1], tangents[2]});

    ASSERT_EQ(m.tangents(), tangents);
}

TEST(Mesh, ExpandingVertsCausesNormalsToExpandWithZeroedNormals)
{
    auto const normals = GenerateNormals(6);

    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_normals(normals);
    m.set_vertices(GenerateVertices(12));

    ASSERT_EQ(m.normals(), ResizedVectorCopy(normals, 12, Vec3{}));
}

TEST(Mesh, ShrinkingVertsCausesTexCoordsToShrinkAlso)
{
    auto uvs = GenerateTexCoords(6);

    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_tex_coords(uvs);
    m.set_vertices(GenerateVertices(3));

    ASSERT_EQ(m.tex_coords(), ResizedVectorCopy(uvs, 3));
}

TEST(Mesh, ExpandingVertsCausesTexCoordsToExpandWithZeroedTexCoords)
{
    auto const uvs = GenerateTexCoords(6);

    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_tex_coords(uvs);
    m.set_vertices(GenerateVertices(12));

    ASSERT_EQ(m.tex_coords(), ResizedVectorCopy(uvs, 12, Vec2{}));
}

TEST(Mesh, ShrinkingVertsCausesColorsToShrinkAlso)
{
    auto const colors = GenerateColors(6);

    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_colors(colors);
    m.set_vertices(GenerateVertices(3));

    ASSERT_EQ(m.colors(), ResizedVectorCopy(colors, 3));
}

TEST(Mesh, ExpandingVertsCausesColorsToExpandWithClearColor)
{
    auto const colors = GenerateColors(6);

    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_colors(colors);
    m.set_vertices(GenerateVertices(12));

    ASSERT_EQ(m.colors(), ResizedVectorCopy(colors, 12, Color::clear()));
}

TEST(Mesh, ShrinkingVertsCausesTangentsToShrinkAlso)
{
    auto const tangents = GenerateTangents(6);

    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_tangents(tangents);
    m.set_vertices(GenerateVertices(3));

    auto const expected = ResizedVectorCopy(tangents, 3);
    auto const got = m.tangents();
    ASSERT_EQ(got, expected);
}

TEST(Mesh, ExpandingVertsCausesTangentsToExpandAlsoAsZeroedTangents)
{
    auto const tangents = GenerateTangents(6);

    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_tangents(tangents);
    m.set_vertices(GenerateVertices(12));  // resized

    auto const expected = ResizedVectorCopy(tangents, 12, Vec4{});
    auto const got = m.tangents();
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
    ASSERT_FALSE(m.has_vertices());
    m.set_vertices(originalVerts);
    ASSERT_EQ(m.vertices(), originalVerts);

    // the verts passed to `transform_vertices` should match those returned by getVerts
    std::vector<Vec3> vertsPassedToTransformVerts;
    m.transform_vertices([&vertsPassedToTransformVerts](Vec3 v)
    {
        vertsPassedToTransformVerts.push_back(v);
        return v;
    });
    ASSERT_EQ(vertsPassedToTransformVerts, originalVerts);

    // applying the transformation should return the transformed verts
    m.transform_vertices([&newVerts, i = 0](Vec3) mutable
    {
        return newVerts.at(i++);
    });
    ASSERT_EQ(m.vertices(), newVerts);
}

TEST(Mesh, TransformVertsCausesTransformedMeshToNotBeEqualToInitialMesh)
{
    Mesh const m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transform_vertices([](Vec3 v) { return v; });  // noop transform also triggers this (meshes aren't value-comparable)

    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformVertsWithTransformAppliesTransformToVerts)
{
    // create appropriate transform
    Transform const transform = {
        .scale = Vec3{0.25f},
        .rotation = to_worldspace_rotation_quat(Eulers{90_deg, 0_deg, 0_deg}),
        .position = {1.0f, 0.25f, 0.125f},
    };

    // generate "original" verts
    auto const original = GenerateVertices(30);

    // precompute "expected" verts
    auto const expected = MapToVector(original, [&transform](auto const& p) { return transform_point(transform, p); });

    // create mesh with "original" verts
    Mesh m;
    m.set_vertices(original);

    // then apply the transform
    m.transform_vertices(transform);

    // the mesh's verts should match expectations
    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, TransformVertsWithTransformCausesTransformedMeshToNotBeEqualToInitialMesh)
{
    Mesh const m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transform_vertices(identity<Transform>());  // noop transform also triggers this (meshes aren't value-comparable)

    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformVertsWithMat4AppliesTransformToVerts)
{
    Mat4 const mat = mat4_cast(Transform{
        .scale = Vec3{0.25f},
        .rotation = to_worldspace_rotation_quat(Eulers{90_deg, 0_deg, 0_deg}),
        .position = {1.0f, 0.25f, 0.125f},
    });

    // generate "original" verts
    auto const original = GenerateVertices(30);

    // precompute "expected" verts
    auto const expected = MapToVector(original, [&mat](auto const& p) { return transform_point(mat, p); });

    // create mesh with "original" verts
    Mesh m;
    m.set_vertices(original);

    // then apply the transform
    m.transform_vertices(mat);

    // the mesh's verts should match expectations
    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, TransformVertsWithMat4CausesTransformedMeshToNotBeEqualToInitialMesh)
{
    Mesh const m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transform_vertices(identity<Mat4>());  // noop

    ASSERT_NE(m, copy) << "should be non-equal because mesh equality is reference-based (if it becomes value-based, delete this test)";
}

TEST(Mesh, HasNormalsReturnsFalseForNewlyConstructedMesh)
{
    ASSERT_FALSE(Mesh{}.has_normals());
}

TEST(Mesh, AssigningOnlyNormalsButNoVertsMakesHasNormalsStillReturnFalse)
{
    Mesh m;
    m.set_normals(GenerateNormals(6));
    ASSERT_FALSE(m.has_normals()) << "shouldn't have any normals, because the caller didn't first assign any vertices";
}

TEST(Mesh, SettingEmptyNormalsOnAnEmptyMeshDoesNotCauseHasNormalsToReturnTrue)
{
    Mesh m;
    m.set_vertices({});
    ASSERT_FALSE(m.has_vertices());
    m.set_normals({});
    ASSERT_FALSE(m.has_normals());
}

TEST(Mesh, AssigningNormalsAndThenVerticiesMakesNormalsAssignmentFail)
{
    Mesh m;
    m.set_normals(GenerateNormals(9));
    m.set_vertices(GenerateVertices(9));
    ASSERT_FALSE(m.has_normals()) << "shouldn't have any normals, because the caller assigned the vertices _after_ assigning the normals (must be first)";
}

TEST(Mesh, AssigningVerticesAndThenNormalsMakesHasNormalsReturnTrue)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_normals(GenerateNormals(6));
    ASSERT_TRUE(m.has_normals()) << "this should work: the caller assigned vertices (good) _and then_ normals (also good)";
}

TEST(Mesh, ClearingMeshClearsHasNormals)
{
    Mesh m;
    m.set_vertices(GenerateVertices(3));
    m.set_normals(GenerateNormals(3));
    ASSERT_TRUE(m.has_normals());
    m.clear();
    ASSERT_FALSE(m.has_normals());
}

TEST(Mesh, HasNormalsReturnsFalseIfOnlyAssigningVerts)
{
    Mesh m;
    m.set_vertices(GenerateVertices(3));
    ASSERT_FALSE(m.has_normals()) << "shouldn't have normals: the caller didn't assign any vertices first";
}

TEST(Mesh, GetNormalsReturnsEmptyOnDefaultConstruction)
{
    Mesh m;
    ASSERT_TRUE(m.normals().empty());
}

TEST(Mesh, AssigningOnlyNormalsMakesGetNormalsReturnNothing)
{
    Mesh m;
    m.set_normals(GenerateNormals(3));

    ASSERT_TRUE(m.normals().empty()) << "should be empty, because the caller didn't first assign any vertices";
}

TEST(Mesh, AssigningNormalsAfterVerticesBehavesAsExpected)
{
    Mesh m;
    auto const normals = GenerateNormals(3);

    m.set_vertices(GenerateVertices(3));
    m.set_normals(normals);

    ASSERT_EQ(m.normals(), normals) << "should assign the normals: the caller did what's expected";
}

TEST(Mesh, AssigningFewerNormalsThanVerticesShouldntAssignTheNormals)
{
    Mesh m;
    m.set_vertices(GenerateVertices(9));
    m.set_normals(GenerateNormals(6));  // note: less than num verts
    ASSERT_FALSE(m.has_normals()) << "normals were not assigned: different size from vertices";
}

TEST(Mesh, AssigningMoreNormalsThanVerticesShouldntAssignTheNormals)
{
    Mesh m;
    m.set_vertices(GenerateVertices(9));
    m.set_normals(GenerateNormals(12));
    ASSERT_FALSE(m.has_normals()) << "normals were not assigned: different size from vertices";
}

TEST(Mesh, SuccessfullyAsssigningNormalsChangesMeshEquality)
{
    Mesh m;
    m.set_vertices(GenerateVertices(12));

    Mesh copy{m};
    ASSERT_EQ(m, copy);
    copy.set_normals(GenerateNormals(12));
    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformNormalsTransormsTheNormals)
{
    auto const transform = [](Vec3 n) { return -n; };
    auto const original = GenerateNormals(16);
    auto expected = original;
    std::transform(expected.begin(), expected.end(), expected.begin(), transform);

    Mesh m;
    m.set_vertices(GenerateVertices(16));
    m.set_normals(original);
    ASSERT_EQ(m.normals(), original);
    m.transform_normals(transform);

    auto const returned = m.normals();
    ASSERT_EQ(returned, expected);
}

TEST(Mesh, HasTexCoordsReturnsFalseForDefaultConstructedMesh)
{
    ASSERT_FALSE(Mesh{}.has_tex_coords());
}

TEST(Mesh, AssigningOnlyTexCoordsCausesHasTexCoordsToReturnFalse)
{
    Mesh m;
    m.set_tex_coords(GenerateTexCoords(3));
    ASSERT_FALSE(m.has_tex_coords()) << "texture coordinates not assigned: no vertices";
}

TEST(Mesh, AssigningTexCoordsAndThenVerticesCausesHasTexCoordsToReturnFalse)
{
    Mesh m;
    m.set_tex_coords(GenerateTexCoords(3));
    m.set_vertices(GenerateVertices(3));
    ASSERT_FALSE(m.has_tex_coords()) << "texture coordinates not assigned: assigned in the wrong order";
}

TEST(Mesh, AssigningVerticesAndThenTexCoordsCausesHasTexCoordsToReturnTrue)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_tex_coords(GenerateTexCoords(6));
    ASSERT_TRUE(m.has_tex_coords());
}

TEST(Mesh, AssigningNoVerticesAndThenNoTexCoordsCausesHasTexCoordsToReturnFalse)
{
    Mesh m;
    m.set_vertices({});
    ASSERT_FALSE(m.has_vertices());
    m.set_tex_coords({});
    ASSERT_FALSE(m.has_tex_coords());
}

TEST(Mesh, GetTexCoordsReturnsEmptyOnDefaultConstruction)
{
    Mesh m;
    ASSERT_TRUE(m.tex_coords().empty());
}

TEST(Mesh, GetTexCoordsReturnsEmptyIfNoVerticesToAssignTheTexCoordsTo)
{
    Mesh m;
    m.set_tex_coords(GenerateTexCoords(6));
    ASSERT_TRUE(m.tex_coords().empty());
}

TEST(Mesh, GetTexCoordsReturnsSetCoordinatesWhenUsedNormally)
{
    Mesh m;
    m.set_vertices(GenerateVertices(12));
    auto const coords = GenerateTexCoords(12);
    m.set_tex_coords(coords);
    ASSERT_EQ(m.tex_coords(), coords);
}

TEST(Mesh, SetTexCoordsDoesNotSetCoordsIfGivenLessCoordsThanVerts)
{
    Mesh m;
    m.set_vertices(GenerateVertices(12));
    m.set_tex_coords(GenerateTexCoords(9));  // note: less
    ASSERT_FALSE(m.has_tex_coords());
    ASSERT_TRUE(m.tex_coords().empty());
}

TEST(Mesh, SetTexCoordsDoesNotSetCoordsIfGivenMoreCoordsThanVerts)
{
    Mesh m;
    m.set_vertices(GenerateVertices(12));
    m.set_tex_coords(GenerateTexCoords(15));  // note: more
    ASSERT_FALSE(m.has_tex_coords());
    ASSERT_TRUE(m.tex_coords().empty());
}

TEST(Mesh, SuccessfulSetCoordsCausesCopiedMeshToBeNotEqualToOriginalMesh)
{
    Mesh m;
    m.set_vertices(GenerateVertices(12));
    Mesh copy{m};
    ASSERT_EQ(m, copy);
    copy.set_tex_coords(GenerateTexCoords(12));
    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformTexCoordsAppliesTransformToTexCoords)
{
    auto const transform = [](Vec2 uv) { return 0.287f * uv; };
    auto const original = GenerateTexCoords(3);
    auto expected = original;
    std::transform(expected.begin(), expected.end(), expected.begin(), transform);

    Mesh m;
    m.set_vertices(GenerateVertices(3));
    m.set_tex_coords(original);
    ASSERT_EQ(m.tex_coords(), original);
    m.transform_tex_coords(transform);
    ASSERT_EQ(m.tex_coords(), expected);
}

TEST(Mesh, GetColorsInitiallyReturnsEmptySpan)
{
    ASSERT_TRUE(Mesh{}.colors().empty());
}

TEST(Mesh, GetColorsRemainsEmptyIfAssignedWithNoVerts)
{
    Mesh m;
    ASSERT_TRUE(m.colors().empty());
    m.set_colors(GenerateColors(3));
    ASSERT_TRUE(m.colors().empty()) << "no verticies to assign colors to";
}

TEST(Mesh, GetColorsReturnsSetColorsWhenAssignedToVertices)
{
    Mesh m;
    m.set_vertices(GenerateVertices(9));
    auto const colors = GenerateColors(9);
    m.set_colors(colors);
    ASSERT_FALSE(m.colors().empty());
    ASSERT_EQ(m.colors(), colors);
}

TEST(Mesh, SetColorsAssignmentFailsIfGivenFewerColorsThanVerts)
{
    Mesh m;
    m.set_vertices(GenerateVertices(9));
    m.set_colors(GenerateColors(6));  // note: less
    ASSERT_TRUE(m.colors().empty());
}

TEST(Mesh, SetColorsAssignmentFailsIfGivenMoreColorsThanVerts)
{
    Mesh m;
    m.set_vertices(GenerateVertices(9));
    m.set_colors(GenerateColors(12));  // note: more
    ASSERT_TRUE(m.colors().empty());
}

TEST(Mesh, GetTangentsInitiallyReturnsEmptySpan)
{
    Mesh m;
    ASSERT_TRUE(m.tangents().empty());
}

TEST(Mesh, SetTangentsFailsWhenAssigningWithNoVerts)
{
    Mesh m;
    m.set_tangents(GenerateTangents(3));
    ASSERT_TRUE(m.tangents().empty());
}

TEST(Mesh, SetTangentsWorksWhenAssigningToCorrectNumberOfVertices)
{
    Mesh m;
    m.set_vertices(GenerateVertices(15));
    auto const tangents = GenerateTangents(15);
    m.set_tangents(tangents);
    ASSERT_FALSE(m.tangents().empty());
    ASSERT_EQ(m.tangents(), tangents);
}

TEST(Mesh, SetTangentsFailsIfFewerTangentsThanVerts)
{
    Mesh m;
    m.set_vertices(GenerateVertices(15));
    m.set_tangents(GenerateTangents(12));  // note: fewer
    ASSERT_TRUE(m.tangents().empty());
}

TEST(Mesh, SetTangentsFailsIfMoreTangentsThanVerts)
{
    Mesh m;
    m.set_vertices(GenerateVertices(15));
    m.set_tangents(GenerateTangents(18));  // note: more
    ASSERT_TRUE(m.tangents().empty());
}

TEST(Mesh, GetNumIndicesReturnsZeroOnDefaultConstruction)
{
    Mesh m;
    ASSERT_EQ(m.num_indices(), 0);
}

TEST(Mesh, GetNumIndicesReturnsNumberOfAssignedIndices)
{
    auto const verts = GenerateVertices(3);
    auto const indices = GenerateIndices(0, 3);

    Mesh m;
    m.set_vertices(verts);
    m.set_indices(indices);

    ASSERT_EQ(m.num_indices(), 3);
}

TEST(Mesh, SetIndiciesWithNoFlagsWorksForNormalArgs)
{
    auto const indices = GenerateIndices(0, 3);

    Mesh m;
    m.set_vertices(GenerateVertices(3));
    m.set_indices(indices);

    ASSERT_EQ(m.num_indices(), 3);
}

TEST(Mesh, SetIndicesCanBeCalledWithInitializerList)
{
    Mesh m;
    m.set_vertices(GenerateVertices(3));
    m.set_indices({0, 1, 2});
    std::vector<uint32_t> const expected = {0, 1, 2};
    auto const got = m.indices();

    ASSERT_TRUE(std::equal(got.begin(), got.end(), expected.begin(), expected.end()));
}

TEST(Mesh, SetIndicesAlsoWorksIfOnlyIndexesSomeOfTheVerts)
{
    auto const indices = GenerateIndices(3, 6);  // only indexes half the verts

    Mesh m;
    m.set_vertices(GenerateVertices(6));
    ASSERT_NO_THROW({ m.set_indices(indices); });
}

TEST(Mesh, SetIndicesThrowsIfOutOfBounds)
{
    Mesh m;
    m.set_vertices(GenerateVertices(3));
    ASSERT_ANY_THROW({ m.set_indices(GenerateIndices(3, 6)); }) << "should throw: indices are out-of-bounds";
}

TEST(Mesh, SetIndiciesWithDontValidateIndicesAndDontRecalculateBounds)
{
    Mesh m;
    m.set_vertices(GenerateVertices(3));
    ASSERT_NO_THROW({ m.set_indices(GenerateIndices(3, 6), MeshUpdateFlags::DontValidateIndices | MeshUpdateFlags::DontRecalculateBounds); }) << "shouldn't throw: we explicitly asked the engine to not check indices";
}

TEST(Mesh, SetIndicesRecalculatesBounds)
{
    Triangle const triangle = GenerateTriangle();

    Mesh m;
    m.set_vertices(triangle);
    ASSERT_EQ(m.bounds(), AABB{});
    m.set_indices(GenerateIndices(0, 3));
    ASSERT_EQ(m.bounds(), bounding_aabb_of(triangle));
}

TEST(Mesh, SetIndicesWithDontRecalculateBoundsDoesNotRecalculateBounds)
{
    Triangle const triangle = GenerateTriangle();

    Mesh m;
    m.set_vertices(triangle);
    ASSERT_EQ(m.bounds(), AABB{});
    m.set_indices(GenerateIndices(0, 3), MeshUpdateFlags::DontRecalculateBounds);
    ASSERT_EQ(m.bounds(), AABB{}) << "bounds shouldn't update: we explicitly asked for the engine to skip it";
}

TEST(Mesh, ForEachIndexedVertNotCalledWithEmptyMesh)
{
    size_t ncalls = 0;
    Mesh{}.for_each_indexed_vert([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, ForEachIndexedVertNotCalledWhenOnlyVertexDataSupplied)
{
    Mesh m;
    m.set_vertices({Vec3{}, Vec3{}, Vec3{}});
    size_t ncalls = 0;
    m.for_each_indexed_vert([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, ForEachIndexedVertCalledWhenSuppliedCorrectlyIndexedMesh)
{
    Mesh m;
    m.set_vertices({Vec3{}, Vec3{}, Vec3{}});
    m.set_indices(std::to_array<uint16_t>({0, 1, 2}));
    size_t ncalls = 0;
    m.for_each_indexed_vert([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 3);
}

TEST(Mesh, ForEachIndexedVertCalledEvenWhenMeshIsNonTriangular)
{
    Mesh m;
    m.set_topology(MeshTopology::Lines);
    m.set_vertices({Vec3{}, Vec3{}, Vec3{}, Vec3{}});
    m.set_indices(std::to_array<uint16_t>({0, 1, 2, 3}));
    size_t ncalls = 0;
    m.for_each_indexed_vert([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 4);
}

TEST(Mesh, ForEachIndexedTriangleNotCalledWithEmptyMesh)
{
    size_t ncalls = 0;
    Mesh{}.for_each_indexed_triangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, ForEachIndexedTriangleNotCalledWhenMeshhasNoIndicies)
{
    Mesh m;
    m.set_vertices({Vec3{}, Vec3{}, Vec3{}});  // unindexed
    size_t ncalls = 0;
    m.for_each_indexed_triangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, ForEachIndexedTriangleCalledIfMeshContainsIndexedTriangles)
{
    Mesh m;
    m.set_vertices({Vec3{}, Vec3{}, Vec3{}});
    m.set_indices(std::to_array<uint16_t>({0, 1, 2}));
    size_t ncalls = 0;
    m.for_each_indexed_triangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 1);
}

TEST(Mesh, ForEachIndexedTriangleNotCalledIfMeshContainsInsufficientIndices)
{
    Mesh m;
    m.set_vertices({Vec3{}, Vec3{}, Vec3{}});
    m.set_indices(std::to_array<uint16_t>({0, 1}));  // too few
    size_t ncalls = 0;
    m.for_each_indexed_triangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, ForEachIndexedTriangleCalledMultipleTimesForMultipleTriangles)
{
    Mesh m;
    m.set_vertices({Vec3{}, Vec3{}, Vec3{}});
    m.set_indices(std::to_array<uint16_t>({0, 1, 2, 1, 2, 0}));
    size_t ncalls = 0;
    m.for_each_indexed_triangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 2);
}

TEST(Mesh, ForEachIndexedTriangleNotCalledIfMeshTopologyIsLines)
{
    Mesh m;
    m.set_topology(MeshTopology::Lines);
    m.set_vertices({Vec3{}, Vec3{}, Vec3{}});
    m.set_indices(std::to_array<uint16_t>({0, 1, 2, 1, 2, 0}));
    size_t ncalls = 0;
    m.for_each_indexed_triangle([&ncalls](auto&&) { ++ncalls; });
    ASSERT_EQ(ncalls, 0);
}

TEST(Mesh, GetTriangleAtReturnsExpectedTriangleForNormalCase)
{
    Triangle const t = GenerateTriangle();

    Mesh m;
    m.set_vertices(t);
    m.set_indices(std::to_array<uint16_t>({0, 1, 2}));

    ASSERT_EQ(m.get_triangle_at(0), t);
}

TEST(Mesh, GetTriangleAtReturnsTriangleIndexedByIndiciesAtProvidedOffset)
{
    Triangle const a = GenerateTriangle();
    Triangle const b = GenerateTriangle();

    Mesh m;
    m.set_vertices({a[0], a[1], a[2], b[0], b[1], b[2]});             // stored as  [a, b]
    m.set_indices(std::to_array<uint16_t>({3, 4, 5, 0, 1, 2}));  // indexed as [b, a]

    ASSERT_EQ(m.get_triangle_at(0), b) << "the provided arg is an offset into the _indices_";
    ASSERT_EQ(m.get_triangle_at(3), a) << "the provided arg is an offset into the _indices_";
}

TEST(Mesh, GetTriangleAtThrowsIfCalledOnNonTriangularMesh)
{
    Mesh m;
    m.set_topology(MeshTopology::Lines);
    m.set_vertices({GenerateVec3(), GenerateVec3(), GenerateVec3(), GenerateVec3(), GenerateVec3(), GenerateVec3()});
    m.set_indices(std::to_array<uint16_t>({0, 1, 2, 3, 4, 5}));

    ASSERT_ANY_THROW({ m.get_triangle_at(0); }) << "incorrect topology";
}

TEST(Mesh, GetTriangleAtThrowsIfGivenOutOfBoundsIndexOffset)
{
    Triangle const t = GenerateTriangle();

    Mesh m;
    m.set_vertices(t);
    m.set_indices(std::to_array<uint16_t>({0, 1, 2}));

    ASSERT_ANY_THROW({ m.get_triangle_at(1); }) << "should throw: it's out-of-bounds";
    ASSERT_ANY_THROW({ m.get_triangle_at(2); }) << "should throw: it's out-of-bounds";
    ASSERT_ANY_THROW({ m.get_triangle_at(3); }) << "should throw: it's out-of-bounds";
}

TEST(Mesh, GetIndexedVertsReturnsEmptyArrayForBlankMesh)
{
    ASSERT_TRUE(Mesh{}.indexed_vertices().empty());
}

TEST(Mesh, GetIndexedVertsReturnsEmptyArrayForMeshWithVertsButNoIndices)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));

    ASSERT_TRUE(m.indexed_vertices().empty());
}

TEST(Mesh, GetIndexedVertsReturnsOnlyTheIndexedVerts)
{
    auto const allVerts = GenerateVertices(12);
    auto const subIndices = GenerateIndices(5, 8);

    Mesh m;
    m.set_vertices(allVerts);
    m.set_indices(subIndices);

    auto const expected = MapToVector(std::span{allVerts}.subspan(5, 3), std::identity{});
    auto const got = m.indexed_vertices();

    ASSERT_EQ(m.indexed_vertices(), expected);
}

TEST(Mesh, GetBoundsReturnsEmptyBoundsOnInitialization)
{
    Mesh m;
    AABB empty{};
    ASSERT_EQ(m.bounds(), empty);
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
    m.set_vertices(pyramid);
    AABB empty{};
    ASSERT_EQ(m.bounds(), empty);
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
    m.set_vertices(pyramid);
    m.set_indices(pyramidIndices);
    ASSERT_EQ(m.bounds(), bounding_aabb_of(pyramid));
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
    ASSERT_EQ(Mesh{}.num_submesh_descriptors(), 0);
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
    m.set_vertices(pyramid);
    m.set_indices(pyramidIndices);

    ASSERT_EQ(m.num_submesh_descriptors(), 0);
}

TEST(Mesh, PushSubMeshDescriptorMakesGetMeshSubCountIncrease)
{
    Mesh m;
    ASSERT_EQ(m.num_submesh_descriptors(), 0);
    m.push_submesh_descriptor(SubMeshDescriptor{0, 10, MeshTopology::Triangles});
    ASSERT_EQ(m.num_submesh_descriptors(), 1);
    m.push_submesh_descriptor(SubMeshDescriptor{5, 30, MeshTopology::Lines});
    ASSERT_EQ(m.num_submesh_descriptors(), 2);
}

TEST(Mesh, PushSubMeshDescriptorMakesGetSubMeshDescriptorReturnPushedDescriptor)
{
    Mesh m;
    SubMeshDescriptor const descriptor{0, 10, MeshTopology::Triangles};

    ASSERT_EQ(m.num_submesh_descriptors(), 0);
    m.push_submesh_descriptor(descriptor);
    ASSERT_EQ(m.submesh_descriptor_at(0), descriptor);
}

TEST(Mesh, PushSecondDescriptorMakesGetReturnExpectedResults)
{
    Mesh m;
    SubMeshDescriptor const firstDesc{0, 10, MeshTopology::Triangles};
    SubMeshDescriptor const secondDesc{5, 15, MeshTopology::Lines};

    m.push_submesh_descriptor(firstDesc);
    m.push_submesh_descriptor(secondDesc);

    ASSERT_EQ(m.num_submesh_descriptors(), 2);
    ASSERT_EQ(m.submesh_descriptor_at(0), firstDesc);
    ASSERT_EQ(m.submesh_descriptor_at(1), secondDesc);
}

TEST(Mesh, SetSubmeshDescriptorsWithRangeWorksAsExpected)
{
    Mesh m;
    SubMeshDescriptor const firstDesc{0, 10, MeshTopology::Triangles};
    SubMeshDescriptor const secondDesc{5, 15, MeshTopology::Lines};

    m.set_submesh_descriptors(std::vector{firstDesc, secondDesc});

    ASSERT_EQ(m.num_submesh_descriptors(), 2);
    ASSERT_EQ(m.submesh_descriptor_at(0), firstDesc);
    ASSERT_EQ(m.submesh_descriptor_at(1), secondDesc);
}

TEST(Mesh, SetSubmeshDescriptorsRemovesExistingDescriptors)
{
    SubMeshDescriptor const firstDesc{0, 10, MeshTopology::Triangles};
    SubMeshDescriptor const secondDesc{5, 15, MeshTopology::Lines};
    SubMeshDescriptor const thirdDesc{20, 35, MeshTopology::Triangles};

    Mesh m;
    m.push_submesh_descriptor(firstDesc);

    ASSERT_EQ(m.num_submesh_descriptors(), 1);
    ASSERT_EQ(m.submesh_descriptor_at(0), firstDesc);

    m.set_submesh_descriptors(std::vector{secondDesc, thirdDesc});

    ASSERT_EQ(m.num_submesh_descriptors(), 2);
    ASSERT_EQ(m.submesh_descriptor_at(0), secondDesc);
    ASSERT_EQ(m.submesh_descriptor_at(1), thirdDesc);
}

TEST(Mesh, GetSubMeshDescriptorThrowsOOBExceptionIfOOBAccessed)
{
    Mesh m;

    ASSERT_EQ(m.num_submesh_descriptors(), 0);
    ASSERT_ANY_THROW({ m.submesh_descriptor_at(0); });

    m.push_submesh_descriptor({0, 10, MeshTopology::Triangles});
    ASSERT_EQ(m.num_submesh_descriptors(), 1);
    ASSERT_NO_THROW({ m.submesh_descriptor_at(0); });
    ASSERT_ANY_THROW({ m.submesh_descriptor_at(1); });
}

TEST(Mesh, ClearSubMeshDescriptorsDoesNothingOnEmptyMesh)
{
    Mesh m;
    ASSERT_NO_THROW({ m.clear_submesh_descriptors(); });
}

TEST(Mesh, ClearSubMeshDescriptorsClearsAllDescriptors)
{
    Mesh m;
    m.push_submesh_descriptor({0, 10, MeshTopology::Triangles});
    m.push_submesh_descriptor({5, 15, MeshTopology::Lines});

    ASSERT_EQ(m.num_submesh_descriptors(), 2);
    ASSERT_NO_THROW({ m.clear_submesh_descriptors(); });
    ASSERT_EQ(m.num_submesh_descriptors(), 0);
}

TEST(Mesh, GeneralClearMethodAlsoClearsSubMeshDescriptors)
{
    Mesh m;
    m.push_submesh_descriptor({0, 10, MeshTopology::Triangles});
    m.push_submesh_descriptor({5, 15, MeshTopology::Lines});

    ASSERT_EQ(m.num_submesh_descriptors(), 2);
    ASSERT_NO_THROW({ m.clear(); });
    ASSERT_EQ(m.num_submesh_descriptors(), 0);
}

TEST(Mesh, GetVertexAttributeCountInitiallyZero)
{
    ASSERT_EQ(Mesh{}.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountBecomes1AfterSettingVerts)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    ASSERT_EQ(m.num_vertex_attributes(), 1);
}

TEST(Mesh, GetVertexAttributeCountRezeroesIfVerticesAreCleared)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_vertices({});
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountGoesToTwoAfterSetting)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_normals(GenerateNormals(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
}

TEST(Mesh, GetVertexAttributeCountGoesDownAsExpectedWrtNormals)
{
    Mesh m;
    m.set_vertices(GenerateVertices(12));
    m.set_normals(GenerateNormals(12));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_normals({});  // clear normals: should only clear the normals
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_normals(GenerateNormals(12));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_vertices({});  // clear verts: should clear vertices + attributes (here: normals)
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountIsZeroAfterClearingWholeMeshWrtNormals)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_normals(GenerateNormals(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.clear();
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountGoesToTwoAfterAssigningTexCoords)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_tex_coords(GenerateTexCoords(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
}

TEST(Mesh, GetVertexAttributeCountGoesBackToOneAfterClearingTexCoords)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_tex_coords(GenerateTexCoords(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_tex_coords({}); // clear them
    ASSERT_EQ(m.num_vertex_attributes(), 1);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedWrtTexCoords)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_tex_coords(GenerateTexCoords(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_vertices({});
    ASSERT_EQ(m.num_vertex_attributes(), 0);
    m.set_vertices(GenerateVertices(6));
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_tex_coords(GenerateTexCoords(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.clear();
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedWrtColors)
{
    Mesh m;
    m.set_vertices(GenerateVertices(12));
    m.set_colors(GenerateColors(12));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_colors({});
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_colors(GenerateColors(12));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_vertices({});
    ASSERT_EQ(m.num_vertex_attributes(), 0);
    m.set_vertices(GenerateVertices(12));
    m.set_colors(GenerateColors(12));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.clear();
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedWrtTangents)
{
    Mesh m;
    ASSERT_EQ(m.num_vertex_attributes(), 0);
    m.set_vertices(GenerateVertices(9));
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_tangents(GenerateTangents(9));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_tangents({});
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_tangents(GenerateTangents(9));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_vertices({});
    ASSERT_EQ(m.num_vertex_attributes(), 0);
    m.set_vertices(GenerateVertices(9));
    m.set_tangents(GenerateTangents(9));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.clear();
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedForMultipleAttributes)
{
    Mesh m;

    // first, try adding all possible attributes
    ASSERT_EQ(m.num_vertex_attributes(), 0);
    m.set_vertices(GenerateVertices(6));
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_normals(GenerateNormals(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_tex_coords(GenerateTexCoords(6));
    ASSERT_EQ(m.num_vertex_attributes(), 3);
    m.set_colors(GenerateColors(6));
    ASSERT_EQ(m.num_vertex_attributes(), 4);
    m.set_tangents(GenerateTangents(6));
    ASSERT_EQ(m.num_vertex_attributes(), 5);

    // then make sure that assigning over them doesn't change
    // the number of attributes (i.e. it's an in-place assignment)
    ASSERT_EQ(m.num_vertex_attributes(), 5);
    m.set_vertices(GenerateVertices(6));
    ASSERT_EQ(m.num_vertex_attributes(), 5);
    m.set_normals(GenerateNormals(6));
    ASSERT_EQ(m.num_vertex_attributes(), 5);
    m.set_tex_coords(GenerateTexCoords(6));
    ASSERT_EQ(m.num_vertex_attributes(), 5);
    m.set_colors(GenerateColors(6));
    ASSERT_EQ(m.num_vertex_attributes(), 5);
    m.set_tangents(GenerateTangents(6));
    ASSERT_EQ(m.num_vertex_attributes(), 5);

    // then make sure that attributes can be deleted in a different
    // order from assignment, and attribute count behaves as-expected
    {
        Mesh copy = m;
        ASSERT_EQ(copy.num_vertex_attributes(), 5);
        copy.set_tex_coords({});
        ASSERT_EQ(copy.num_vertex_attributes(), 4);
        copy.set_colors({});
        ASSERT_EQ(copy.num_vertex_attributes(), 3);
        copy.set_normals({});
        ASSERT_EQ(copy.num_vertex_attributes(), 2);
        copy.set_tangents({});
        ASSERT_EQ(copy.num_vertex_attributes(), 1);
        copy.set_vertices({});
        ASSERT_EQ(copy.num_vertex_attributes(), 0);
    }

    // ... and Mesh::clear behaves as expected
    {
        Mesh copy = m;
        ASSERT_EQ(copy.num_vertex_attributes(), 5);
        copy.clear();
        ASSERT_EQ(copy.num_vertex_attributes(), 0);
    }

    // ... and clearing the verts first clears all attributes
    {
        Mesh copy = m;
        ASSERT_EQ(copy.num_vertex_attributes(), 5);
        copy.set_vertices({});
        ASSERT_EQ(copy.num_vertex_attributes(), 0);
    }
}

TEST(Mesh, GetVertexAttributesReturnsEmptyOnConstruction)
{
    ASSERT_TRUE(Mesh{}.vertex_format().empty());
}

TEST(Mesh, GetVertexAttributesReturnsExpectedAttributeWhenJustVerticesAreSet)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));

    VertexFormat const expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    };

    ASSERT_EQ(m.vertex_format(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedAttributesWhenVerticesAndNormalsSet)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_normals(GenerateNormals(6));

    VertexFormat const expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,   VertexAttributeFormat::Float32x3},
    };

    ASSERT_EQ(m.vertex_format(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedWhenVerticesAndTexCoordsSet)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_tex_coords(GenerateTexCoords(6));

    VertexFormat const expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_EQ(m.vertex_format(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedWhenVerticesAndColorsSet)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_colors(GenerateColors(6));

    VertexFormat const expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Color,    VertexAttributeFormat::Float32x4},
    };

    ASSERT_EQ(m.vertex_format(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedWhenVerticesAndTangentsSet)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_tangents(GenerateTangents(6));

    VertexFormat const expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent,  VertexAttributeFormat::Float32x4},
    };

    ASSERT_EQ(m.vertex_format(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedForCombinations)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_normals(GenerateNormals(6));
    m.set_tex_coords(GenerateTexCoords(6));

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
        };
        ASSERT_EQ(m.vertex_format(), expected);
    }

    m.set_colors(GenerateColors(6));

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
        };
        ASSERT_EQ(m.vertex_format(), expected);
    }

    m.set_tangents(GenerateTangents(6));

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
        };
        ASSERT_EQ(m.vertex_format(), expected);
    }

    m.set_colors({});  // clear color

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
        };
        ASSERT_EQ(m.vertex_format(), expected);
    }

    m.set_colors(GenerateColors(6));

    // check that ordering is based on when it was set
    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
        };
        ASSERT_EQ(m.vertex_format(), expected);
    }

    m.set_normals({});

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
        };
        ASSERT_EQ(m.vertex_format(), expected);
    }

    Mesh copy{m};

    {
        VertexFormat const expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
        };
        ASSERT_EQ(copy.vertex_format(), expected);
    }

    m.set_vertices({});

    {
        VertexFormat const expected;
        ASSERT_EQ(m.vertex_format(), expected);
        ASSERT_NE(copy.vertex_format(), expected) << "the copy should be independent";
    }

    copy.clear();

    {
        VertexFormat const expected;
        ASSERT_EQ(copy.vertex_format(), expected);
    }
}

TEST(Mesh, SetVertexBufferParamsWithEmptyDescriptorIgnoresN)
{
    Mesh m;
    m.set_vertices(GenerateVertices(9));

    ASSERT_EQ(m.num_vertices(), 9);
    ASSERT_EQ(m.num_vertex_attributes(), 1);

    m.set_vertex_buffer_params(15, {});  // i.e. no data, incl. positions

    ASSERT_EQ(m.num_vertices(), 0);  // i.e. the 15 was effectively ignored, because there's no attributes
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, SetVertexBufferParamsWithEmptyDescriptorClearsAllAttributesNotJustPosition)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_normals(GenerateNormals(6));
    m.set_colors(GenerateColors(6));

    ASSERT_EQ(m.num_vertices(), 6);
    ASSERT_EQ(m.num_vertex_attributes(), 3);

    m.set_vertex_buffer_params(24, {});

    ASSERT_EQ(m.num_vertices(), 0);
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, SetVertexBufferParamsExpandsPositionsWithZeroedVectors)
{
    auto const verts = GenerateVertices(6);

    Mesh m;
    m.set_vertices(verts);
    m.set_vertex_buffer_params(12, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3}
    });

    auto expected = verts;
    expected.resize(12, Vec3{});

    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, SetVertexBufferParamsCanShrinkPositionVectors)
{
    auto const verts = GenerateVertices(12);

    Mesh m;
    m.set_vertices(verts);
    m.set_vertex_buffer_params(6, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3}
    });

    auto expected = verts;
    expected.resize(6);

    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, SetVertexBufferParamsWhenDimensionalityOfVerticesIs2ZeroesTheMissingDimension)
{
    auto const verts = GenerateVertices(6);

    Mesh m;
    m.set_vertices(verts);
    m.set_vertex_buffer_params(6, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},  // 2D storage
    });

    auto const expected = MapToVector(verts, [](Vec3 const& v) { return Vec3{v.x, v.y, 0.0f}; });

    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, SetVertexBufferParamsCanBeUsedToRemoveAParticularAttribute)
{
    auto const verts = GenerateVertices(6);
    auto const tangents = GenerateTangents(6);

    Mesh m;
    m.set_vertices(verts);
    m.set_normals(GenerateNormals(6));
    m.set_tangents(tangents);

    ASSERT_EQ(m.num_vertex_attributes(), 3);

    m.set_vertex_buffer_params(6, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent,  VertexAttributeFormat::Float32x4},
        // i.e. remove the normals
    });

    ASSERT_EQ(m.num_vertices(), 6);
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    ASSERT_EQ(m.vertices(), verts);
    ASSERT_EQ(m.tangents(), tangents);
}

TEST(Mesh, SetVertexBufferParamsCanBeUsedToAddAParticularAttributeAsZeroed)
{
    auto const verts = GenerateVertices(6);
    auto const tangents = GenerateTangents(6);

    Mesh m;
    m.set_vertices(verts);
    m.set_tangents(tangents);

    ASSERT_EQ(m.num_vertex_attributes(), 2);

    m.set_vertex_buffer_params(6, {
        // existing
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
        // new
        {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    });

    ASSERT_EQ(m.vertices(), verts);
    ASSERT_EQ(m.tangents(), tangents);
    ASSERT_EQ(m.colors(), std::vector<Color>(6));
    ASSERT_EQ(m.tex_coords(), std::vector<Vec2>(6));
}

TEST(Mesh, SetVertexBufferParamsThrowsIfItCausesIndicesToGoOutOfBounds)
{
    Mesh m;
    m.set_vertices(GenerateVertices(6));
    m.set_indices(GenerateIndices(0, 6));

    ASSERT_ANY_THROW({ m.set_vertex_buffer_params(3, m.vertex_format()); })  << "should throw because indices are now OOB";
}

TEST(Mesh, SetVertexBufferParamsCanBeUsedToReformatToU8NormFormat)
{
    auto const colors = GenerateColors(9);

    Mesh m;
    m.set_vertices(GenerateVertices(9));
    m.set_colors(colors);

    ASSERT_EQ(m.colors(), colors);

    m.set_vertex_buffer_params(9, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Color,    VertexAttributeFormat::Unorm8x4},
    });

    auto const expected = MapToVector(colors, [](Color const& c) {
        return to_color(to_color32(c));
    });

    ASSERT_EQ(m.colors(), expected);
}

TEST(Mesh, GetVertexBufferStrideReturnsExpectedResults)
{
    Mesh m;
    ASSERT_EQ(m.vertex_buffer_stride(), 0);

    m.set_vertex_buffer_params(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    });
    ASSERT_EQ(m.vertex_buffer_stride(), 3*sizeof(float));

    m.set_vertex_buffer_params(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},
    });
    ASSERT_EQ(m.vertex_buffer_stride(), 2*sizeof(float));

    m.set_vertex_buffer_params(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},
        {VertexAttribute::Color,    VertexAttributeFormat::Float32x4},
    });
    ASSERT_EQ(m.vertex_buffer_stride(), 2*sizeof(float)+4*sizeof(float));

    m.set_vertex_buffer_params(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},
        {VertexAttribute::Color,    VertexAttributeFormat::Unorm8x4},
    });
    ASSERT_EQ(m.vertex_buffer_stride(), 2*sizeof(float)+4);

    m.set_vertex_buffer_params(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::Color,    VertexAttributeFormat::Unorm8x4},
    });
    ASSERT_EQ(m.vertex_buffer_stride(), 4+4);

    m.set_vertex_buffer_params(3, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},
        {VertexAttribute::Tangent,  VertexAttributeFormat::Float32x4},
        {VertexAttribute::Color,    VertexAttributeFormat::Unorm8x4},
    });
    ASSERT_EQ(m.vertex_buffer_stride(), 2*sizeof(float) + 4 + 4*sizeof(float));
}

TEST(Mesh, SetVertexBufferDataWorksForSimplestCase)
{
    struct Entry final {
        Vec3 vert = GenerateVec3();
    };
    std::vector<Entry> const data(12);

    Mesh m;
    m.set_vertex_buffer_params(12, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    });
    m.set_vertex_buffer_data(data);

    auto const expected = MapToVector(data, [](auto const& entry) { return entry.vert; });

    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, SetVertexBufferDataFailsInSimpleCaseIfAttributeMismatches)
{
    struct Entry final {
        Vec3 vert = GenerateVec3();
    };
    std::vector<Entry> const data(12);

    Mesh m;
    m.set_vertex_buffer_params(12, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},  // uh oh: wrong dimensionality for `Entry`
    });
    ASSERT_ANY_THROW({ m.set_vertex_buffer_data(data); });
}

TEST(Mesh, SetVertexBufferDataFailsInSimpleCaseIfNMismatches)
{
    struct Entry final {
        Vec3 vert = GenerateVec3();
    };
    std::vector<Entry> const data(12);

    Mesh m;
    m.set_vertex_buffer_params(6, {  // uh oh: wrong N for the given number of entries
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    });
    ASSERT_ANY_THROW({ m.set_vertex_buffer_data(data); });
}

TEST(Mesh, SetVertexBufferDataDoesntFailIfTheCallerLuckilyProducesSameLayout)
{
    struct Entry final {
        Vec4 vert = GenerateVec4();  // note: Vec4
    };
    std::vector<Entry> const data(12);

    Mesh m;
    m.set_vertex_buffer_params(24, {  // uh oh
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},  // ah, but, the total size will now luckily match...
    });
    ASSERT_NO_THROW({ m.set_vertex_buffer_data(data); });  // and it won't throw because the API cannot know any better...
}

TEST(Mesh, SetVertexBufferDataThrowsIfLayoutNotProvided)
{
    struct Entry final {
        Vec3 verts;
    };
    std::vector<Entry> const data(12);

    Mesh m;
    ASSERT_ANY_THROW({ m.set_vertex_buffer_data(data); }) << "should throw: caller didn't call 'set_vertex_buffer_params' first";
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
    auto const expectedColors = MapToVector(data, [](auto const& v) { return to_color(v.col); });
    auto const expectedTexCoords = MapToVector(data, [](auto const& v) { return v.uv; });

    Mesh m;
    m.set_vertex_buffer_params(16, {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x2},
        {VertexAttribute::Color,     VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    });

    // directly set vertex buffer data
    ASSERT_EQ(m.vertex_buffer_stride(), sizeof(SimilarToImGuiVert));
    ASSERT_NO_THROW({ m.set_vertex_buffer_data(data); });

    auto const verts = m.vertices();
    auto const colors = m.colors();
    auto const texCoords = m.tex_coords();

    ASSERT_EQ(verts, expectedVerts);
    ASSERT_EQ(colors, expectedColors);
    ASSERT_EQ(texCoords, expectedTexCoords);
}

TEST(Mesh, SetVertexBufferDataRecalculatesBounds)
{
    auto firstVerts = GenerateVertices(6);
    auto secondVerts = MapToVector(firstVerts, [](auto const& v) { return 2.0f*v; });  // i.e. has different bounds

    Mesh m;
    m.set_vertices(firstVerts);
    m.set_indices(GenerateIndices(0, 6));

    ASSERT_EQ(m.bounds(), bounding_aabb_of(firstVerts));

    m.set_vertex_buffer_data(secondVerts);

    ASSERT_EQ(m.bounds(), bounding_aabb_of(secondVerts));
}

TEST(Mesh, RecalculateNormalsDoesNothingIfTopologyIsLines)
{
    Mesh m;
    m.set_vertices(GenerateVertices(2));
    m.set_indices({0, 1});
    m.set_topology(MeshTopology::Lines);

    ASSERT_FALSE(m.has_normals());
    m.recalculate_normals();
    ASSERT_FALSE(m.has_normals()) << "shouldn't recalculate for lines";
}

TEST(Mesh, RecalculateNormalsAssignsNormalsIfNoneExist)
{
    Mesh m;
    m.set_vertices({  // i.e. triangle that's wound to point in +Z
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    });
    m.set_indices({0, 1, 2});
    ASSERT_FALSE(m.has_normals());
    m.recalculate_normals();
    ASSERT_TRUE(m.has_normals());

    auto const normals = m.normals();
    ASSERT_EQ(normals.size(), 3);
    ASSERT_TRUE(std::all_of(normals.begin(), normals.end(), [first = normals.front()](Vec3 const& normal){ return normal == first; }));
    ASSERT_TRUE(all_of(equal_within_absdiff(normals.front(), Vec3(0.0f, 0.0f, 1.0f), epsilon_v<float>)));
}

TEST(Mesh, RecalculateNormalsSmoothsNormalsOfSharedVerts)
{
    // create a "tent" mesh, where two 45-degree-angled triangles
    // are joined on one edge (two verts) on the top
    //
    // `recalculate_normals` should ensure that the normals at the
    // vertices on the top are calculated by averaging each participating
    // triangle's normals (which point outwards at an angle)

    auto const verts = std::to_array<Vec3>({
        {-1.0f, 0.0f,  0.0f},  // bottom-left "pin"
        { 0.0f, 1.0f,  1.0f},  // front of "top"
        { 0.0f, 1.0f, -1.0f},  // back of "top"
        { 1.0f, 0.0f,  0.0f},  // bottom-right "pin"
    });

    Mesh m;
    m.set_vertices(verts);
    m.set_indices({0, 1, 2,   3, 2, 1});  // shares two verts per triangle

    Vec3 const lhsNormal = triangle_normal({ verts[0], verts[1], verts[2] });
    Vec3 const rhsNormal = triangle_normal({ verts[3], verts[2], verts[1] });
    Vec3 const mixedNormal = normalize(midpoint(lhsNormal, rhsNormal));

    m.recalculate_normals();

    auto const normals = m.normals();
    ASSERT_EQ(normals.size(), 4);
    ASSERT_TRUE(all_of(equal_within_absdiff(normals[0], lhsNormal, epsilon_v<float>)));
    ASSERT_TRUE(all_of(equal_within_absdiff(normals[1], mixedNormal, epsilon_v<float>)));
    ASSERT_TRUE(all_of(equal_within_absdiff(normals[2], mixedNormal, epsilon_v<float>)));
    ASSERT_TRUE(all_of(equal_within_absdiff(normals[3], rhsNormal, epsilon_v<float>)));
}

TEST(Mesh, RecalculateTangentsDoesNothingIfTopologyIsLines)
{
    Mesh m;
    m.set_topology(MeshTopology::Lines);
    m.set_vertices({ GenerateVec3(), GenerateVec3() });
    m.set_normals(GenerateNormals(2));
    m.set_tex_coords(GenerateTexCoords(2));

    ASSERT_TRUE(m.tangents().empty());
    m.recalculate_tangents();
    ASSERT_TRUE(m.tangents().empty()) << "shouldn't do anything if topology is lines";
}

TEST(Mesh, RecalculateTangentsDoesNothingIfNoNormals)
{
    Mesh m;
    m.set_vertices({  // i.e. triangle that's wound to point in +Z
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    });
    // skip normals
    m.set_tex_coords({
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    });
    m.set_indices({0, 1, 2});
    ASSERT_TRUE(m.tangents().empty());
    m.recalculate_tangents();
    ASSERT_TRUE(m.tangents().empty()) << "cannot calculate tangents if normals are missing";
}

TEST(Mesh, RecalculateTangentsDoesNothingIfNoTexCoords)
{
    Mesh m;
    m.set_vertices({  // i.e. triangle that's wound to point in +Z
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    });
    m.set_normals({
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    });
    // no tex coords
    m.set_indices({0, 1, 2});

    ASSERT_TRUE(m.tangents().empty());
    m.recalculate_tangents();
    ASSERT_TRUE(m.tangents().empty()) << "cannot calculate tangents if text coords are missing";
}

TEST(Mesh, RecalculateTangentsDoesNothingIfIndicesAreNotAssigned)
{
    Mesh m;
    m.set_vertices({  // i.e. triangle that's wound to point in +Z
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    });
    m.set_normals({
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    });
    m.set_tex_coords({
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    });
    // no indices

    ASSERT_TRUE(m.tangents().empty());
    m.recalculate_tangents();
    ASSERT_TRUE(m.tangents().empty()) << "cannot recalculate tangents if there are no indices (needed to figure out what's a triangle, etc.)";
}

TEST(Mesh, RecalculateTangentsCreatesTangentsIfNoneExist)
{
    Mesh m;
    m.set_vertices({  // i.e. triangle that's wound to point in +Z
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    });
    m.set_normals({
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    });
    m.set_tex_coords({
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    });
    m.set_indices({0, 1, 2});

    ASSERT_TRUE(m.tangents().empty());
    m.recalculate_tangents();
    ASSERT_FALSE(m.tangents().empty());
}

TEST(Mesh, RecalculateTangentsGivesExpectedResultsInBasicCase)
{
    Mesh m;
    m.set_vertices({  // i.e. triangle that's wound to point in +Z
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    });
    m.set_normals({
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    });
    m.set_tex_coords({
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    });
    m.set_indices({0, 1, 2});

    ASSERT_TRUE(m.tangents().empty());
    m.recalculate_tangents();

    auto const tangents = m.tangents();

    ASSERT_EQ(tangents.size(), 3);
    ASSERT_EQ(tangents.at(0), Vec4(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_EQ(tangents.at(1), Vec4(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_EQ(tangents.at(2), Vec4(1.0f, 0.0f, 0.0f, 0.0f));
}
