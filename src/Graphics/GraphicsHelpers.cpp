#include "GraphicsHelpers.hpp"

#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Tetrahedron.hpp"
#include "src/Platform/App.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>

namespace
{
    // assumes `pos` is in-bounds
    void DrawBVHRecursive(
        std::shared_ptr<osc::Mesh const> const& mesh,
        osc::BVH const& bvh,
        int pos,
        std::vector<osc::SceneDecoration>& out)
    {
        osc::BVHNode const& n = bvh.nodes[pos];

        osc::Transform t;
        t.scale *= 0.5f * Dimensions(n.getBounds());
        t.position = Midpoint(n.getBounds());

        glm::vec4 const color = {0.0f, 0.0f, 0.0f, 1.0f};

        out.emplace_back(mesh, t, color);

        if (n.isNode())
        {
            // it's an internal node
            DrawBVHRecursive(mesh, bvh, pos+1, out);
            DrawBVHRecursive(mesh, bvh, pos+n.getNumLhsNodes()+1, out);
        }
    }

    void DrawGrid(glm::quat const& rotation, std::vector<osc::SceneDecoration>& out)
    {
        std::shared_ptr<osc::Mesh const> const grid = osc::App::meshes().get100x100GridMesh();

        osc::Transform t;
        t.scale *= glm::vec3{50.0f, 50.0f, 1.0f};
        t.rotation = rotation;

        glm::vec4 const color = {0.7f, 0.7f, 0.7f, 0.15f};

        out.emplace_back(grid, t, color);
    }
}

void osc::DrawBVH(BVH const& sceneBVH, std::vector<SceneDecoration>& out)
{
    if (sceneBVH.nodes.empty())
    {
        return;
    }

    std::shared_ptr<Mesh const> const cube = App::meshes().getCubeWireMesh();
    DrawBVHRecursive(cube, sceneBVH, 0, out);
}

void osc::DrawAABB(AABB const& aabb, std::vector<SceneDecoration>& out)
{
    std::shared_ptr<Mesh const> const cube = App::meshes().getCubeWireMesh();
    glm::vec4 const color = {0.0f, 0.0f, 0.0f, 1.0f};

    Transform t;
    t.scale = 0.5f * Dimensions(aabb);
    t.position = Midpoint(aabb);

    out.emplace_back(cube, t, color);
}

void osc::DrawAABBs(nonstd::span<AABB const> aabbs, std::vector<SceneDecoration>& out)
{
    std::shared_ptr<Mesh const> const cube = App::meshes().getCubeWireMesh();
    glm::vec4 const color = {0.0f, 0.0f, 0.0f, 1.0f};

    for (AABB const& aabb : aabbs)
    {
        Transform t;
        t.scale = 0.5f * Dimensions(aabb);
        t.position = Midpoint(aabb);

        out.emplace_back(cube, t, color);
    }
}

void osc::DrawXZFloorLines(std::vector<SceneDecoration>& out, float scale)
{
    std::shared_ptr<Mesh const> const yLine = App::meshes().getYLineMesh();

    // X line
    {
        glm::vec4 const color = {1.0f, 0.0f, 0.0f, 1.0f};

        Transform t;
        t.scale *= scale;
        t.rotation = glm::angleAxis(fpi2, glm::vec3{0.0f, 0.0f, 1.0f});

        out.emplace_back(yLine, t, color);
    }

    // Z line
    {
        glm::vec4 const color = {0.0f, 1.0f, 0.0f, 1.0f};

        Transform t;
        t.scale *= scale;
        t.rotation = glm::angleAxis(fpi2, glm::vec3{1.0f, 0.0f, 0.0f});

        out.emplace_back(yLine, t, color);
    }
}

void osc::DrawXZGrid(std::vector<SceneDecoration>& out)
{
    glm::quat const rotation = glm::angleAxis(fpi2, glm::vec3{1.0f, 0.0f, 0.0f});
    DrawGrid(rotation, out);
}

void osc::DrawXYGrid(std::vector<SceneDecoration>& out)
{
    glm::quat const rotation = glm::identity<glm::quat>();
    DrawGrid(rotation, out);
}

void osc::DrawYZGrid(std::vector<SceneDecoration>& out)
{
    glm::quat rotation = glm::angleAxis(fpi2, glm::vec3{0.0f, 1.0f, 0.0f});
    DrawGrid(rotation, out);
}

void osc::UpdateSceneBVH(nonstd::span<SceneDecoration const> sceneEls, BVH& bvh)
{
    std::vector<AABB> aabbs;
    aabbs.reserve(sceneEls.size());

    for (auto const& el : sceneEls)
    {
        aabbs.push_back(GetWorldspaceAABB(el));
    }

    bvh.buildFromAABBs(aabbs);
}

// returns all collisions along a ray
std::vector<osc::SceneCollision> osc::GetAllSceneCollisions(BVH const& bvh, nonstd::span<SceneDecoration const> decorations, Line const& ray)
{
    // use scene BVH to intersect the ray with the scene
    std::vector<BVHCollision> const sceneCollisions = bvh.getRayAABBCollisions(ray);

    // perform ray-triangle intersections tests on the scene hits
    std::vector<SceneCollision> rv;
    for (BVHCollision const& c : sceneCollisions)
    {
        SceneDecoration const& decoration = decorations[c.id];
        std::optional<RayCollision> const maybeCollision = GetClosestWorldspaceRayCollision(*decoration.mesh, decoration.transform, ray);

        if (maybeCollision)
        {
            rv.emplace_back(maybeCollision->position, static_cast<size_t>(c.id), maybeCollision->distance);
        }
    }
    return rv;
}

std::optional<osc::RayCollision> osc::GetClosestWorldspaceRayCollision(Mesh const& mesh, Transform const& transform, Line const& worldspaceRay)
{
    if (mesh.getTopography() != MeshTopography::Triangles)
    {
        return std::nullopt;
    }

    // map the ray into the mesh's modelspace, so that we compute a ray-mesh collision
    Line const modelspaceRay = InverseTransformLine(worldspaceRay, transform);

    MeshIndicesView const indices = mesh.getIndices();
    std::optional<BVHCollision> const maybeCollision = indices.isU16() ?
        mesh.getBVH().getClosestRayIndexedTriangleCollision(mesh.getVerts(), indices.toU16Span(), modelspaceRay) :
        mesh.getBVH().getClosestRayIndexedTriangleCollision(mesh.getVerts(), indices.toU32Span(), modelspaceRay);

    if (maybeCollision)
    {
        // map the ray back into worldspace
        glm::vec3 const locationWorldspace = transform * maybeCollision->position;
        float const distance = glm::length(locationWorldspace - worldspaceRay.origin);
        return osc::RayCollision{distance, locationWorldspace};
    }
    else
    {
        return std::nullopt;
    }
}

glm::vec3 osc::MassCenter(Mesh const& m)
{
    // hastily implemented from: http://forums.cgsociety.org/t/how-to-calculate-center-of-mass-for-triangular-mesh/1309966
    //
    // effectively:
    //
    // - compute the centerpoint and volume of tetrahedrons created from
    //   some arbitrary point in space to each triangle in the mesh
    //
    // - compute the weighted sum: sum(volume * center) / sum(volume)
    //
    // this yields a 3D location that is a "mass center", *but* the volume
    // calculation is signed based on vertex winding (normal), so if the user
    // submits an invalid mesh, this calculation could potentially produce a
    // volume that's *way* off

    if (m.getTopography() != osc::MeshTopography::Triangles)
    {
        return {0.0f, 0.0f, 0.0f};
    }

    nonstd::span<glm::vec3 const> const verts = m.getVerts();
    MeshIndicesView const indices = m.getIndices();
    size_t const len = (indices.size() / 3) * 3;  // paranioa

    double totalVolume = 0.0f;
    glm::vec<3, double> weightedCenterOfMass = {0.0, 0.0, 0.0};
    for (size_t i = 0; i < len; i += 3)
    {
        Tetrahedron tetrahedron;
        tetrahedron[0] = {0.0f, 0.0f, 0.0f};  // reference point
        tetrahedron[1] = verts[indices[i]];
        tetrahedron[2] = verts[indices[i+1]];
        tetrahedron[3] = verts[indices[i+2]];

        double const volume = Volume(tetrahedron);
        glm::vec<3, double> const centerOfMass = Center(tetrahedron);

        totalVolume += volume;
        weightedCenterOfMass += volume * centerOfMass;
    }
    return weightedCenterOfMass / totalVolume;
}

glm::vec3 osc::AverageCenterpoint(Mesh const& m)
{
    MeshIndicesView const indices = m.getIndices();
    nonstd::span<glm::vec3 const> const verts = m.getVerts();

    glm::vec3 acc = {0.0f, 0.0f, 0.0f};
    for (uint32_t index : indices)
    {
        acc += verts[index];
    }
    acc /= static_cast<float>(verts.size());

    return acc;
}

osc::Material osc::CreateWireframeOverlayMaterial()
{
    osc::Material material{osc::ShaderCache::get("shaders/SceneSolidColor.vert", "shaders/SceneSolidColor.frag")};
    material.setVec4("uDiffuseColor", {0.0f, 0.0f, 0.0f, 0.6f});
    material.setWireframeMode(true);
    material.setTransparent(true);
    return material;
}