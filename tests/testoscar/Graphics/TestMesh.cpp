#include <oscar/Graphics/Mesh.h>

#include <testoscar/TestingHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/SubMeshDescriptor.h>
#include <oscar/Graphics/VertexFormat.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/EulerAngles.h>
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
#include <iterator>
#include <random>
#include <span>
#include <sstream>
#include <utility>
#include <vector>

using namespace osc;
using namespace osc::literals;
using namespace osc::testing;

TEST(Mesh, can_be_default_constructed)
{
    const Mesh mesh;
}

TEST(Mesh, can_be_copy_constructed)
{
    const Mesh m;
    Mesh{m};
}

TEST(Mesh, can_be_move_constructed)
{
    Mesh m1;
    const Mesh m2{std::move(m1)};
}

TEST(Mesh, can_be_copy_assigned)
{
    Mesh m1;
    const Mesh m2;

    m1 = m2;
}

TEST(Mesh, can_be_move_assigned)
{
    Mesh m1;
    Mesh m2;

    m1 = std::move(m2);
}

TEST(Mesh, can_call_topology)
{
    const Mesh m;

    m.topology();
}

TEST(Mesh, topology_defaults_to_Triangles)
{
    const Mesh m;

    ASSERT_EQ(m.topology(), MeshTopology::Triangles);
}

TEST(Mesh, set_topology_causes_topology_to_return_new_MeshTopology)
{
    Mesh m;
    const auto newTopology = MeshTopology::Lines;

    ASSERT_NE(m.topology(), MeshTopology::Lines);

    m.set_topology(newTopology);

    ASSERT_EQ(m.topology(), newTopology);
}

TEST(Mesh, set_topology_causes_copied_Mesh_to_compare_not_equal_to_initial_Mesh)
{
    const Mesh m;
    Mesh copy{m};
    const auto newTopology = MeshTopology::Lines;

    ASSERT_EQ(m, copy);
    ASSERT_NE(copy.topology(), newTopology);

    copy.set_topology(newTopology);

    ASSERT_NE(m, copy);
}

TEST(Mesh, num_vertices_initially_returns_zero)
{
    ASSERT_EQ(Mesh{}.num_vertices(), 0);
}

TEST(Mesh, set_vertices_causes_num_vertices_to_return_the_number_of_set_vertices)
{
    Mesh m;
    m.set_vertices(generate_vertices(3));
    ASSERT_EQ(m.num_vertices(), 3);
}

TEST(Mesh, has_vertices_initially_returns_false)
{
    ASSERT_FALSE(Mesh{}.has_vertices());
}

TEST(Mesh, has_vertices_returns_true_after_setting_vertices)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    ASSERT_TRUE(m.has_vertices());
}

TEST(Mesh, GetVertsReturnsEmptyVertsOnDefaultConstruction)
{
    ASSERT_TRUE(Mesh{}.vertices().empty());
}

TEST(Mesh, set_vertices_makes_vertices_return_the_vertices)
{
    Mesh m;
    const auto verts = generate_vertices(9);

    m.set_vertices(verts);

    ASSERT_EQ(m.vertices(), verts);
}

TEST(Mesh, set_vertices_can_be_called_with_an_initializer_list_of_vertices)
{
    Mesh m;

    Vec3 a{};
    Vec3 b{};
    Vec3 c{};

    m.set_vertices({a, b, c});
    std::vector<Vec3> expected = {a, b, c};

    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, set_vertices_can_be_called_with_UnitVec3_because_of_implicit_conversion)
{
    Mesh m;
    UnitVec3 v{1.0f, 0.0f, 0.0f};
    m.set_vertices({v});
    std::vector<Vec3> expected = {v};
    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, set_vertices_causes_copied_Mesh_to_compare_not_equal_to_initial_Mesh)
{
    const Mesh m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.set_vertices(generate_vertices(30));

    ASSERT_NE(m, copy);
}

TEST(Mesh, shrinking_vertices_also_shrinks_normals)
{
    const auto normals = generate_normals(6);

    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_normals(normals);
    m.set_vertices(generate_vertices(3));

    ASSERT_EQ(m.normals(), resized_vector_copy(normals, 3));
}

TEST(Mesh, set_normals_can_be_called_with_an_initializer_list)
{
    const auto verts = generate_vertices(3);
    const auto normals = generate_normals(3);

    Mesh m;
    m.set_vertices(verts);
    m.set_normals({normals[0], normals[1], normals[2]});

    ASSERT_EQ(m.normals(), normals);
}

TEST(Mesh, set_tex_coords_can_be_called_with_an_initializer_list)
{
    const auto verts = generate_vertices(3);
    const auto uvs = generate_texture_coordinates(3);

    Mesh m;
    m.set_vertices(verts);
    m.set_tex_coords({uvs[0], uvs[1], uvs[2]});

    ASSERT_EQ(m.tex_coords(), uvs);
}

TEST(Mesh, set_colors_can_be_called_with_an_initializer_list)
{
    const auto verts = generate_vertices(3);
    const auto colors = generate_colors(3);

    Mesh m;
    m.set_vertices(verts);
    m.set_colors({colors[0], colors[1], colors[2]});

    ASSERT_EQ(m.colors(), colors);
}

TEST(Mesh, set_tangents_can_be_called_with_an_initializer_list)
{
    const auto verts = generate_vertices(3);
    const auto tangents = generate_tangent_vectors(3);

    Mesh m;
    m.set_vertices(verts);
    m.set_tangents({tangents[0], tangents[1], tangents[2]});

    ASSERT_EQ(m.tangents(), tangents);
}

TEST(Mesh, expanding_vertices_also_expands_normals_with_zeroed_normals)
{
    const auto normals = generate_normals(6);

    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_normals(normals);
    m.set_vertices(generate_vertices(12));

    ASSERT_EQ(m.normals(), resized_vector_copy(normals, 12));
}

TEST(Mesh, shrinking_vertices_also_shrinks_tex_coords)
{
    const auto uvs = generate_texture_coordinates(6);

    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_tex_coords(uvs);
    m.set_vertices(generate_vertices(3));

    ASSERT_EQ(m.tex_coords(), resized_vector_copy(uvs, 3));
}

TEST(Mesh, expanding_vertices_also_expands_tex_coords_with_zeroed_tex_coords)
{
    const auto uvs = generate_texture_coordinates(6);

    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_tex_coords(uvs);
    m.set_vertices(generate_vertices(12));

    ASSERT_EQ(m.tex_coords(), resized_vector_copy(uvs, 12));
}

TEST(Mesh, shrinking_vertices_also_shrinks_colors)
{
    const auto colors = generate_colors(6);

    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_colors(colors);
    m.set_vertices(generate_vertices(3));

    ASSERT_EQ(m.colors(), resized_vector_copy(colors, 3));
}

TEST(Mesh, expanding_vertices_also_expands_colors_with_clear_color)
{
    const auto colors = generate_colors(6);

    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_colors(colors);
    m.set_vertices(generate_vertices(12));

    ASSERT_EQ(m.colors(), resized_vector_copy(colors, 12, Color::clear()));
}

TEST(Mesh, shrinking_vertices_also_shrinks_tangents)
{
    const auto tangents = generate_tangent_vectors(6);

    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_tangents(tangents);
    m.set_vertices(generate_vertices(3));

    const auto expected = resized_vector_copy(tangents, 3);
    const auto got = m.tangents();
    ASSERT_EQ(got, expected);
}

TEST(Mesh, expanding_vertices_also_expands_tangents_with_zeroed_tangents)
{
    const auto tangents = generate_tangent_vectors(6);

    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_tangents(tangents);
    m.set_vertices(generate_vertices(12));  // resized

    ASSERT_EQ(m.tangents(), resized_vector_copy(tangents, 12));
}

TEST(Mesh, transform_certices_makes_vertices_return_transformed_vertices)
{
    Mesh m;

    // generate "original" verts
    const auto original_vertices = generate_vertices(30);

    // create "transformed" version of the verts
    const auto new_vertices = project_into_vector(original_vertices, [](const Vec3& v) { return v + 1.0f; });

    // sanity check that `set_vertices` works as expected
    ASSERT_FALSE(m.has_vertices());
    m.set_vertices(original_vertices);
    ASSERT_EQ(m.vertices(), original_vertices);

    // the vertices passed to `transform_vertices` should match those returned by `vertices()`
    std::vector<Vec3> vertices_passed_to_transform_vertices;
    m.transform_vertices([&vertices_passed_to_transform_vertices](Vec3 v)
    {
        vertices_passed_to_transform_vertices.push_back(v);
        return v;
    });
    ASSERT_EQ(vertices_passed_to_transform_vertices, original_vertices);

    // applying the transformation should return the transformed verts
    m.transform_vertices([&new_vertices, i = 0](Vec3) mutable
    {
        return new_vertices.at(i++);
    });
    ASSERT_EQ(m.vertices(), new_vertices);
}

TEST(Mesh, transform_vertices_causes_transformed_Mesh_to_compare_not_equal_to_original_Mesh)
{
    const Mesh m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transform_vertices([](Vec3 v) { return v; });  // noop transform also triggers this (meshes aren't value-comparable)

    ASSERT_NE(m, copy);
}

TEST(Mesh, transform_vertices_with_Transform_applies_Transform_to_each_vertex)
{
    // create appropriate transform
    const Transform transform = {
        .scale = Vec3{0.25f},
        .rotation = to_worldspace_rotation_quat(EulerAngles{90_deg, 0_deg, 0_deg}),
        .position = {1.0f, 0.25f, 0.125f},
    };

    // generate "original" verts
    const auto original = generate_vertices(30);

    // precompute "expected" verts
    const auto expected = project_into_vector(original, [&transform](const auto& p) { return transform_point(transform, p); });

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
    const Mesh m;
    Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transform_vertices(identity<Transform>());  // noop transform also triggers this (meshes aren't value-comparable)

    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformVertsWithMat4AppliesTransformToVerts)
{
    const Mat4 mat = mat4_cast(Transform{
        .scale = Vec3{0.25f},
        .rotation = to_worldspace_rotation_quat(EulerAngles{90_deg, 0_deg, 0_deg}),
        .position = {1.0f, 0.25f, 0.125f},
    });

    // generate "original" verts
    const auto original = generate_vertices(30);

    // precompute "expected" verts
    const auto expected = project_into_vector(original, [&mat](const auto& p) { return transform_point(mat, p); });

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
    const Mesh m;
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
    m.set_normals(generate_normals(6));
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
    m.set_normals(generate_normals(9));
    m.set_vertices(generate_vertices(9));
    ASSERT_FALSE(m.has_normals()) << "shouldn't have any normals, because the caller assigned the vertices _after_ assigning the normals (must be first)";
}

TEST(Mesh, AssigningVerticesAndThenNormalsMakesHasNormalsReturnTrue)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_normals(generate_normals(6));
    ASSERT_TRUE(m.has_normals()) << "this should work: the caller assigned vertices (good) _and then_ normals (also good)";
}

TEST(Mesh, ClearingMeshClearsHasNormals)
{
    Mesh m;
    m.set_vertices(generate_vertices(3));
    m.set_normals(generate_normals(3));
    ASSERT_TRUE(m.has_normals());
    m.clear();
    ASSERT_FALSE(m.has_normals());
}

TEST(Mesh, HasNormalsReturnsFalseIfOnlyAssigningVerts)
{
    Mesh m;
    m.set_vertices(generate_vertices(3));
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
    m.set_normals(generate_normals(3));

    ASSERT_TRUE(m.normals().empty()) << "should be empty, because the caller didn't first assign any vertices";
}

TEST(Mesh, AssigningNormalsAfterVerticesBehavesAsExpected)
{
    Mesh m;
    const auto normals = generate_normals(3);

    m.set_vertices(generate_vertices(3));
    m.set_normals(normals);

    ASSERT_EQ(m.normals(), normals) << "should assign the normals: the caller did what's expected";
}

TEST(Mesh, AssigningFewerNormalsThanVerticesShouldntAssignTheNormals)
{
    Mesh m;
    m.set_vertices(generate_vertices(9));
    m.set_normals(generate_normals(6));  // note: less than num verts
    ASSERT_FALSE(m.has_normals()) << "normals were not assigned: different size from vertices";
}

TEST(Mesh, AssigningMoreNormalsThanVerticesShouldntAssignTheNormals)
{
    Mesh m;
    m.set_vertices(generate_vertices(9));
    m.set_normals(generate_normals(12));
    ASSERT_FALSE(m.has_normals()) << "normals were not assigned: different size from vertices";
}

TEST(Mesh, SuccessfullyAsssigningNormalsChangesMeshEquality)
{
    Mesh m;
    m.set_vertices(generate_vertices(12));

    Mesh copy{m};
    ASSERT_EQ(m, copy);
    copy.set_normals(generate_normals(12));
    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformNormalsTransormsTheNormals)
{
    const auto transform = [](Vec3 n) { return -n; };
    const auto original = generate_normals(16);
    auto expected = original;
    std::transform(expected.begin(), expected.end(), expected.begin(), transform);

    Mesh m;
    m.set_vertices(generate_vertices(16));
    m.set_normals(original);
    ASSERT_EQ(m.normals(), original);
    m.transform_normals(transform);

    const auto returned = m.normals();
    ASSERT_EQ(returned, expected);
}

TEST(Mesh, HasTexCoordsReturnsFalseForDefaultConstructedMesh)
{
    ASSERT_FALSE(Mesh{}.has_tex_coords());
}

TEST(Mesh, AssigningOnlyTexCoordsCausesHasTexCoordsToReturnFalse)
{
    Mesh m;
    m.set_tex_coords(generate_texture_coordinates(3));
    ASSERT_FALSE(m.has_tex_coords()) << "texture coordinates not assigned: no vertices";
}

TEST(Mesh, AssigningTexCoordsAndThenVerticesCausesHasTexCoordsToReturnFalse)
{
    Mesh m;
    m.set_tex_coords(generate_texture_coordinates(3));
    m.set_vertices(generate_vertices(3));
    ASSERT_FALSE(m.has_tex_coords()) << "texture coordinates not assigned: assigned in the wrong order";
}

TEST(Mesh, AssigningVerticesAndThenTexCoordsCausesHasTexCoordsToReturnTrue)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_tex_coords(generate_texture_coordinates(6));
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
    m.set_tex_coords(generate_texture_coordinates(6));
    ASSERT_TRUE(m.tex_coords().empty());
}

TEST(Mesh, GetTexCoordsReturnsSetCoordinatesWhenUsedNormally)
{
    Mesh m;
    m.set_vertices(generate_vertices(12));
    const auto coords = generate_texture_coordinates(12);
    m.set_tex_coords(coords);
    ASSERT_EQ(m.tex_coords(), coords);
}

TEST(Mesh, SetTexCoordsDoesNotSetCoordsIfGivenLessCoordsThanVerts)
{
    Mesh m;
    m.set_vertices(generate_vertices(12));
    m.set_tex_coords(generate_texture_coordinates(9));  // note: less
    ASSERT_FALSE(m.has_tex_coords());
    ASSERT_TRUE(m.tex_coords().empty());
}

TEST(Mesh, SetTexCoordsDoesNotSetCoordsIfGivenMoreCoordsThanVerts)
{
    Mesh m;
    m.set_vertices(generate_vertices(12));
    m.set_tex_coords(generate_texture_coordinates(15));  // note: more
    ASSERT_FALSE(m.has_tex_coords());
    ASSERT_TRUE(m.tex_coords().empty());
}

TEST(Mesh, SuccessfulSetCoordsCausesCopiedMeshToBeNotEqualToOriginalMesh)
{
    Mesh m;
    m.set_vertices(generate_vertices(12));
    Mesh copy{m};
    ASSERT_EQ(m, copy);
    copy.set_tex_coords(generate_texture_coordinates(12));
    ASSERT_NE(m, copy);
}

TEST(Mesh, TransformTexCoordsAppliesTransformToTexCoords)
{
    const auto transform = [](Vec2 uv) { return 0.287f * uv; };
    const auto original = generate_texture_coordinates(3);
    auto expected = original;
    std::transform(expected.begin(), expected.end(), expected.begin(), transform);

    Mesh m;
    m.set_vertices(generate_vertices(3));
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
    m.set_colors(generate_colors(3));
    ASSERT_TRUE(m.colors().empty()) << "no verticies to assign colors to";
}

TEST(Mesh, GetColorsReturnsSetColorsWhenAssignedToVertices)
{
    Mesh m;
    m.set_vertices(generate_vertices(9));
    const auto colors = generate_colors(9);
    m.set_colors(colors);
    ASSERT_FALSE(m.colors().empty());
    ASSERT_EQ(m.colors(), colors);
}

TEST(Mesh, SetColorsAssignmentFailsIfGivenFewerColorsThanVerts)
{
    Mesh m;
    m.set_vertices(generate_vertices(9));
    m.set_colors(generate_colors(6));  // note: less
    ASSERT_TRUE(m.colors().empty());
}

TEST(Mesh, SetColorsAssignmentFailsIfGivenMoreColorsThanVerts)
{
    Mesh m;
    m.set_vertices(generate_vertices(9));
    m.set_colors(generate_colors(12));  // note: more
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
    m.set_tangents(generate_tangent_vectors(3));
    ASSERT_TRUE(m.tangents().empty());
}

TEST(Mesh, SetTangentsWorksWhenAssigningToCorrectNumberOfVertices)
{
    Mesh m;
    m.set_vertices(generate_vertices(15));
    const auto tangents = generate_tangent_vectors(15);
    m.set_tangents(tangents);
    ASSERT_FALSE(m.tangents().empty());
    ASSERT_EQ(m.tangents(), tangents);
}

TEST(Mesh, SetTangentsFailsIfFewerTangentsThanVerts)
{
    Mesh m;
    m.set_vertices(generate_vertices(15));
    m.set_tangents(generate_tangent_vectors(12));  // note: fewer
    ASSERT_TRUE(m.tangents().empty());
}

TEST(Mesh, SetTangentsFailsIfMoreTangentsThanVerts)
{
    Mesh m;
    m.set_vertices(generate_vertices(15));
    m.set_tangents(generate_tangent_vectors(18));  // note: more
    ASSERT_TRUE(m.tangents().empty());
}

TEST(Mesh, GetNumIndicesReturnsZeroOnDefaultConstruction)
{
    Mesh m;
    ASSERT_EQ(m.num_indices(), 0);
}

TEST(Mesh, GetNumIndicesReturnsNumberOfAssignedIndices)
{
    const auto verts = generate_vertices(3);
    const auto indices = iota_index_range(0, 3);

    Mesh m;
    m.set_vertices(verts);
    m.set_indices(indices);

    ASSERT_EQ(m.num_indices(), 3);
}

TEST(Mesh, SetIndiciesWithNoFlagsWorksForNormalArgs)
{
    const auto indices = iota_index_range(0, 3);

    Mesh m;
    m.set_vertices(generate_vertices(3));
    m.set_indices(indices);

    ASSERT_EQ(m.num_indices(), 3);
}

TEST(Mesh, SetIndicesCanBeCalledWithInitializerList)
{
    Mesh m;
    m.set_vertices(generate_vertices(3));
    m.set_indices({0, 1, 2});
    const std::vector<uint32_t> expected = {0, 1, 2};
    const auto got = m.indices();

    ASSERT_TRUE(std::equal(got.begin(), got.end(), expected.begin(), expected.end()));
}

TEST(Mesh, SetIndicesAlsoWorksIfOnlyIndexesSomeOfTheVerts)
{
    const auto indices = iota_index_range(3, 6);  // only indexes half the verts

    Mesh m;
    m.set_vertices(generate_vertices(6));
    ASSERT_NO_THROW({ m.set_indices(indices); });
}

TEST(Mesh, SetIndicesThrowsIfOutOfBounds)
{
    Mesh m;
    m.set_vertices(generate_vertices(3));
    ASSERT_ANY_THROW({ m.set_indices(iota_index_range(3, 6)); }) << "should throw: indices are out-of-bounds";
}

TEST(Mesh, SetIndices16BitWorksWithEmptyVector)
{
    std::vector<uint16_t> indices;
    Mesh m;
    m.set_vertices(generate_vertices(3));
    m.set_indices(indices);  // should just work
    ASSERT_TRUE(m.indices().empty());
}

TEST(Mesh, SetIndices32BitWorksWithEmptyVector)
{
    std::vector<uint32_t> indices;
    Mesh m;
    m.set_vertices(generate_vertices(3));
    m.set_indices(indices);  // should just work
    ASSERT_TRUE(m.indices().empty());
}

TEST(Mesh, SetIndiciesWithDontValidateIndicesAndDontRecalculateBounds)
{
    Mesh m;
    m.set_vertices(generate_vertices(3));
    ASSERT_NO_THROW({ m.set_indices(iota_index_range(3, 6), MeshUpdateFlags::DontValidateIndices | MeshUpdateFlags::DontRecalculateBounds); }) << "shouldn't throw: we explicitly asked the engine to not check indices";
}

TEST(Mesh, SetIndicesRecalculatesBounds)
{
    const Triangle triangle = generate<Triangle>();

    Mesh m;
    m.set_vertices(triangle);
    ASSERT_EQ(m.bounds(), AABB{});
    m.set_indices(iota_index_range(0, 3));
    ASSERT_EQ(m.bounds(), bounding_aabb_of(triangle));
}

TEST(Mesh, SetIndicesWithDontRecalculateBoundsDoesNotRecalculateBounds)
{
    const Triangle triangle = generate<Triangle>();

    Mesh m;
    m.set_vertices(triangle);
    ASSERT_EQ(m.bounds(), AABB{});
    m.set_indices(iota_index_range(0, 3), MeshUpdateFlags::DontRecalculateBounds);
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
    const Triangle t = generate<Triangle>();

    Mesh m;
    m.set_vertices(t);
    m.set_indices(std::to_array<uint16_t>({0, 1, 2}));

    ASSERT_EQ(m.get_triangle_at(0), t);
}

TEST(Mesh, GetTriangleAtReturnsTriangleIndexedByIndiciesAtProvidedOffset)
{
    const Triangle a = generate<Triangle>();
    const Triangle b = generate<Triangle>();

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
    m.set_vertices({generate<Vec3>(), generate<Vec3>(), generate<Vec3>(), generate<Vec3>(), generate<Vec3>(), generate<Vec3>()});
    m.set_indices(std::to_array<uint16_t>({0, 1, 2, 3, 4, 5}));

    ASSERT_ANY_THROW({ m.get_triangle_at(0); }) << "incorrect topology";
}

TEST(Mesh, GetTriangleAtThrowsIfGivenOutOfBoundsIndexOffset)
{
    const Triangle t = generate<Triangle>();

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
    m.set_vertices(generate_vertices(6));

    ASSERT_TRUE(m.indexed_vertices().empty());
}

TEST(Mesh, GetIndexedVertsReturnsOnlyTheIndexedVerts)
{
    const auto allVerts = generate_vertices(12);
    const auto subIndices = iota_index_range(5, 8);

    Mesh m;
    m.set_vertices(allVerts);
    m.set_indices(subIndices);

    const auto expected = project_into_vector(std::span{allVerts}.subspan(5, 3), std::identity{});
    const auto got = m.indexed_vertices();

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
    const auto pyramid = std::to_array<Vec3>({
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
    const auto pyramid = std::to_array<Vec3>({
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
    });
    const auto pyramidIndices = std::to_array<uint16_t>({0, 1, 2});

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
    const auto pyramid = std::to_array<Vec3>({
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
    });
    const auto pyramidIndices = std::to_array<uint16_t>({0, 1, 2});

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
    const SubMeshDescriptor descriptor{0, 10, MeshTopology::Triangles};

    ASSERT_EQ(m.num_submesh_descriptors(), 0);
    m.push_submesh_descriptor(descriptor);
    ASSERT_EQ(m.submesh_descriptor_at(0), descriptor);
}

TEST(Mesh, PushSecondDescriptorMakesGetReturnExpectedResults)
{
    Mesh m;
    const SubMeshDescriptor firstDesc{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor secondDesc{5, 15, MeshTopology::Lines};

    m.push_submesh_descriptor(firstDesc);
    m.push_submesh_descriptor(secondDesc);

    ASSERT_EQ(m.num_submesh_descriptors(), 2);
    ASSERT_EQ(m.submesh_descriptor_at(0), firstDesc);
    ASSERT_EQ(m.submesh_descriptor_at(1), secondDesc);
}

TEST(Mesh, SetSubmeshDescriptorsWithRangeWorksAsExpected)
{
    Mesh m;
    const SubMeshDescriptor firstDesc{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor secondDesc{5, 15, MeshTopology::Lines};

    m.set_submesh_descriptors(std::vector{firstDesc, secondDesc});

    ASSERT_EQ(m.num_submesh_descriptors(), 2);
    ASSERT_EQ(m.submesh_descriptor_at(0), firstDesc);
    ASSERT_EQ(m.submesh_descriptor_at(1), secondDesc);
}

TEST(Mesh, SetSubmeshDescriptorsRemovesExistingDescriptors)
{
    const SubMeshDescriptor firstDesc{0, 10, MeshTopology::Triangles};
    const SubMeshDescriptor secondDesc{5, 15, MeshTopology::Lines};
    const SubMeshDescriptor thirdDesc{20, 35, MeshTopology::Triangles};

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
    m.set_vertices(generate_vertices(6));
    ASSERT_EQ(m.num_vertex_attributes(), 1);
}

TEST(Mesh, GetVertexAttributeCountRezeroesIfVerticesAreCleared)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_vertices({});
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountGoesToTwoAfterSetting)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_normals(generate_normals(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
}

TEST(Mesh, GetVertexAttributeCountGoesDownAsExpectedWrtNormals)
{
    Mesh m;
    m.set_vertices(generate_vertices(12));
    m.set_normals(generate_normals(12));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_normals({});  // clear normals: should only clear the normals
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_normals(generate_normals(12));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_vertices({});  // clear verts: should clear vertices + attributes (here: normals)
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountIsZeroAfterClearingWholeMeshWrtNormals)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_normals(generate_normals(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.clear();
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountGoesToTwoAfterAssigningTexCoords)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_tex_coords(generate_texture_coordinates(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
}

TEST(Mesh, GetVertexAttributeCountGoesBackToOneAfterClearingTexCoords)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_tex_coords(generate_texture_coordinates(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_tex_coords({}); // clear them
    ASSERT_EQ(m.num_vertex_attributes(), 1);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedWrtTexCoords)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_tex_coords(generate_texture_coordinates(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_vertices({});
    ASSERT_EQ(m.num_vertex_attributes(), 0);
    m.set_vertices(generate_vertices(6));
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_tex_coords(generate_texture_coordinates(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.clear();
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedWrtColors)
{
    Mesh m;
    m.set_vertices(generate_vertices(12));
    m.set_colors(generate_colors(12));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_colors({});
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_colors(generate_colors(12));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_vertices({});
    ASSERT_EQ(m.num_vertex_attributes(), 0);
    m.set_vertices(generate_vertices(12));
    m.set_colors(generate_colors(12));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.clear();
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedWrtTangents)
{
    Mesh m;
    ASSERT_EQ(m.num_vertex_attributes(), 0);
    m.set_vertices(generate_vertices(9));
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_tangents(generate_tangent_vectors(9));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_tangents({});
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_tangents(generate_tangent_vectors(9));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_vertices({});
    ASSERT_EQ(m.num_vertex_attributes(), 0);
    m.set_vertices(generate_vertices(9));
    m.set_tangents(generate_tangent_vectors(9));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.clear();
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, GetVertexAttributeCountBehavesAsExpectedForMultipleAttributes)
{
    Mesh m;

    // first, try adding all possible attributes
    ASSERT_EQ(m.num_vertex_attributes(), 0);
    m.set_vertices(generate_vertices(6));
    ASSERT_EQ(m.num_vertex_attributes(), 1);
    m.set_normals(generate_normals(6));
    ASSERT_EQ(m.num_vertex_attributes(), 2);
    m.set_tex_coords(generate_texture_coordinates(6));
    ASSERT_EQ(m.num_vertex_attributes(), 3);
    m.set_colors(generate_colors(6));
    ASSERT_EQ(m.num_vertex_attributes(), 4);
    m.set_tangents(generate_tangent_vectors(6));
    ASSERT_EQ(m.num_vertex_attributes(), 5);

    // then make sure that assigning over them doesn't change
    // the number of attributes (i.e. it's an in-place assignment)
    ASSERT_EQ(m.num_vertex_attributes(), 5);
    m.set_vertices(generate_vertices(6));
    ASSERT_EQ(m.num_vertex_attributes(), 5);
    m.set_normals(generate_normals(6));
    ASSERT_EQ(m.num_vertex_attributes(), 5);
    m.set_tex_coords(generate_texture_coordinates(6));
    ASSERT_EQ(m.num_vertex_attributes(), 5);
    m.set_colors(generate_colors(6));
    ASSERT_EQ(m.num_vertex_attributes(), 5);
    m.set_tangents(generate_tangent_vectors(6));
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
    m.set_vertices(generate_vertices(6));

    const VertexFormat expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    };

    ASSERT_EQ(m.vertex_format(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedAttributesWhenVerticesAndNormalsSet)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_normals(generate_normals(6));

    const VertexFormat expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,   VertexAttributeFormat::Float32x3},
    };

    ASSERT_EQ(m.vertex_format(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedWhenVerticesAndTexCoordsSet)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_tex_coords(generate_texture_coordinates(6));

    const VertexFormat expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_EQ(m.vertex_format(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedWhenVerticesAndColorsSet)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_colors(generate_colors(6));

    const VertexFormat expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Color,    VertexAttributeFormat::Float32x4},
    };

    ASSERT_EQ(m.vertex_format(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedWhenVerticesAndTangentsSet)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_tangents(generate_tangent_vectors(6));

    const VertexFormat expected = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent,  VertexAttributeFormat::Float32x4},
    };

    ASSERT_EQ(m.vertex_format(), expected);
}

TEST(Mesh, GetVertexAttributesReturnsExpectedForCombinations)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_normals(generate_normals(6));
    m.set_tex_coords(generate_texture_coordinates(6));

    {
        const VertexFormat expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
        };
        ASSERT_EQ(m.vertex_format(), expected);
    }

    m.set_colors(generate_colors(6));

    {
        const VertexFormat expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
        };
        ASSERT_EQ(m.vertex_format(), expected);
    }

    m.set_tangents(generate_tangent_vectors(6));

    {
        const VertexFormat expected = {
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
        const VertexFormat expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
        };
        ASSERT_EQ(m.vertex_format(), expected);
    }

    m.set_colors(generate_colors(6));

    // check that ordering is based on when it was set
    {
        const VertexFormat expected = {
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
        const VertexFormat expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
        };
        ASSERT_EQ(m.vertex_format(), expected);
    }

    Mesh copy{m};

    {
        const VertexFormat expected = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Tangent,   VertexAttributeFormat::Float32x4},
            {VertexAttribute::Color,     VertexAttributeFormat::Float32x4},
        };
        ASSERT_EQ(copy.vertex_format(), expected);
    }

    m.set_vertices({});

    {
        const VertexFormat expected;
        ASSERT_EQ(m.vertex_format(), expected);
        ASSERT_NE(copy.vertex_format(), expected) << "the copy should be independent";
    }

    copy.clear();

    {
        const VertexFormat expected;
        ASSERT_EQ(copy.vertex_format(), expected);
    }
}

TEST(Mesh, SetVertexBufferParamsWithEmptyDescriptorIgnoresN)
{
    Mesh m;
    m.set_vertices(generate_vertices(9));

    ASSERT_EQ(m.num_vertices(), 9);
    ASSERT_EQ(m.num_vertex_attributes(), 1);

    m.set_vertex_buffer_params(15, {});  // i.e. no data, incl. positions

    ASSERT_EQ(m.num_vertices(), 0);  // i.e. the 15 was effectively ignored, because there's no attributes
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, SetVertexBufferParamsWithEmptyDescriptorClearsAllAttributesNotJustPosition)
{
    Mesh m;
    m.set_vertices(generate_vertices(6));
    m.set_normals(generate_normals(6));
    m.set_colors(generate_colors(6));

    ASSERT_EQ(m.num_vertices(), 6);
    ASSERT_EQ(m.num_vertex_attributes(), 3);

    m.set_vertex_buffer_params(24, {});

    ASSERT_EQ(m.num_vertices(), 0);
    ASSERT_EQ(m.num_vertex_attributes(), 0);
}

TEST(Mesh, SetVertexBufferParamsExpandsPositionsWithZeroedVectors)
{
    const auto verts = generate_vertices(6);

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
    const auto verts = generate_vertices(12);

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
    const auto verts = generate_vertices(6);

    Mesh m;
    m.set_vertices(verts);
    m.set_vertex_buffer_params(6, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},  // 2D storage
    });

    const auto expected = project_into_vector(verts, [](const Vec3& v) { return Vec3{v.x, v.y, 0.0f}; });

    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, SetVertexBufferParamsCanBeUsedToRemoveAParticularAttribute)
{
    const auto verts = generate_vertices(6);
    const auto tangents = generate_tangent_vectors(6);

    Mesh m;
    m.set_vertices(verts);
    m.set_normals(generate_normals(6));
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
    const auto verts = generate_vertices(6);
    const auto tangents = generate_tangent_vectors(6);

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
    m.set_vertices(generate_vertices(6));
    m.set_indices(iota_index_range(0, 6));

    ASSERT_ANY_THROW({ m.set_vertex_buffer_params(3, m.vertex_format()); })  << "should throw because indices are now OOB";
}

TEST(Mesh, SetVertexBufferParamsCanBeUsedToReformatToU8NormFormat)
{
    const auto colors = generate_colors(9);

    Mesh m;
    m.set_vertices(generate_vertices(9));
    m.set_colors(colors);

    ASSERT_EQ(m.colors(), colors);

    m.set_vertex_buffer_params(9, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Color,    VertexAttributeFormat::Unorm8x4},
    });

    const auto expected = project_into_vector(colors, [](const Color& c) {
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
        Vec3 vert = generate<Vec3>();
    };
    const std::vector<Entry> data(12);

    Mesh m;
    m.set_vertex_buffer_params(12, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    });
    m.set_vertex_buffer_data(data);

    const auto expected = project_into_vector(data, [](const auto& entry) { return entry.vert; });

    ASSERT_EQ(m.vertices(), expected);
}

TEST(Mesh, SetVertexBufferDataFailsInSimpleCaseIfAttributeMismatches)
{
    struct Entry final {
        Vec3 vert = generate<Vec3>();
    };
    const std::vector<Entry> data(12);

    Mesh m;
    m.set_vertex_buffer_params(12, {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x2},  // uh oh: wrong dimensionality for `Entry`
    });
    ASSERT_ANY_THROW({ m.set_vertex_buffer_data(data); });
}

TEST(Mesh, SetVertexBufferDataFailsInSimpleCaseIfNMismatches)
{
    struct Entry final {
        Vec3 vert = generate<Vec3>();
    };
    const std::vector<Entry> data(12);

    Mesh m;
    m.set_vertex_buffer_params(6, {  // uh oh: wrong N for the given number of entries
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    });
    ASSERT_ANY_THROW({ m.set_vertex_buffer_data(data); });
}

TEST(Mesh, SetVertexBufferDataDoesntFailIfTheCallerLuckilyProducesSameLayout)
{
    struct Entry final {
        Vec4 vert = generate<Vec4>();  // note: Vec4
    };
    const std::vector<Entry> data(12);

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
    const std::vector<Entry> data(12);

    Mesh m;
    ASSERT_ANY_THROW({ m.set_vertex_buffer_data(data); }) << "should throw: caller didn't call 'set_vertex_buffer_params' first";
}

TEST(Mesh, SetVertexBufferDataWorksAsExpectedForImguiStyleCase)
{
    struct SimilarToImGuiVert final {
        Vec2 pos = generate<Vec2>();
        Color32 col = generate<Color32>();
        Vec2 uv = generate<Vec2>();
    };
    const std::vector<SimilarToImGuiVert> data(16);
    const auto expectedVerts = project_into_vector(data, [](const auto& v) { return Vec3{v.pos, 0.0f}; });
    const auto expectedColors = project_into_vector(data, [](const auto& v) { return to_color(v.col); });
    const auto expectedTexCoords = project_into_vector(data, [](const auto& v) { return v.uv; });

    Mesh m;
    m.set_vertex_buffer_params(16, {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x2},
        {VertexAttribute::Color,     VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    });

    // directly set vertex buffer data
    ASSERT_EQ(m.vertex_buffer_stride(), sizeof(SimilarToImGuiVert));
    ASSERT_NO_THROW({ m.set_vertex_buffer_data(data); });

    const auto verts = m.vertices();
    const auto colors = m.colors();
    const auto texCoords = m.tex_coords();

    ASSERT_EQ(verts, expectedVerts);
    ASSERT_EQ(colors, expectedColors);
    ASSERT_EQ(texCoords, expectedTexCoords);
}

TEST(Mesh, SetVertexBufferDataRecalculatesBounds)
{
    auto firstVerts = generate_vertices(6);
    auto secondVerts = project_into_vector(firstVerts, [](const auto& v) { return 2.0f*v; });  // i.e. has different bounds

    Mesh m;
    m.set_vertices(firstVerts);
    m.set_indices(iota_index_range(0, 6));

    ASSERT_EQ(m.bounds(), bounding_aabb_of(firstVerts));

    m.set_vertex_buffer_data(secondVerts);

    ASSERT_EQ(m.bounds(), bounding_aabb_of(secondVerts));
}

TEST(Mesh, RecalculateNormalsDoesNothingIfTopologyIsLines)
{
    Mesh m;
    m.set_vertices(generate_vertices(2));
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

    const auto normals = m.normals();
    ASSERT_EQ(normals.size(), 3);
    ASSERT_TRUE(std::all_of(normals.begin(), normals.end(), [first = normals.front()](const Vec3& normal){ return normal == first; }));
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

    const auto verts = std::to_array<Vec3>({
        {-1.0f, 0.0f,  0.0f},  // bottom-left "pin"
        { 0.0f, 1.0f,  1.0f},  // front of "top"
        { 0.0f, 1.0f, -1.0f},  // back of "top"
        { 1.0f, 0.0f,  0.0f},  // bottom-right "pin"
    });

    Mesh m;
    m.set_vertices(verts);
    m.set_indices({0, 1, 2,   3, 2, 1});  // shares two verts per triangle

    const Vec3 lhsNormal = triangle_normal({ verts[0], verts[1], verts[2] });
    const Vec3 rhsNormal = triangle_normal({ verts[3], verts[2], verts[1] });
    const Vec3 mixedNormal = normalize(midpoint(lhsNormal, rhsNormal));

    m.recalculate_normals();

    const auto normals = m.normals();
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
    m.set_vertices({ generate<Vec3>(), generate<Vec3>() });
    m.set_normals(generate_normals(2));
    m.set_tex_coords(generate_texture_coordinates(2));

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

    const auto tangents = m.tangents();

    ASSERT_EQ(tangents.size(), 3);
    ASSERT_EQ(tangents.at(0), Vec4(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_EQ(tangents.at(1), Vec4(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_EQ(tangents.at(2), Vec4(1.0f, 0.0f, 0.0f, 0.0f));
}
