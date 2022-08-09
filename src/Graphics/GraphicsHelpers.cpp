#include "GraphicsHelpers.hpp"

#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Platform/App.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>

namespace
{
    // assumes `pos` is in-bounds
    void DrawBVHRecursive(
        std::shared_ptr<osc::experimental::Mesh const> const& mesh,
        osc::BVH const& bvh,
        int pos,
        std::vector<osc::SceneDecoration>& out)
    {
        osc::BVHNode const& n = bvh.nodes[pos];

        osc::Transform t;
        t.scale *= 0.5f * Dimensions(n.bounds);
        t.position = Midpoint(n.bounds);

        glm::vec4 const color = {0.0f, 0.0f, 0.0f, 1.0f};

        out.emplace_back(mesh, t, color);

        if (n.nlhs >= 0)
        {
            // it's an internal node
            DrawBVHRecursive(mesh, bvh, pos+1, out);
            DrawBVHRecursive(mesh, bvh, pos+n.nlhs+1, out);
        }
    }

    void DrawGrid(glm::quat const& rotation, std::vector<osc::SceneDecoration>& out)
    {
        std::shared_ptr<osc::experimental::Mesh const> grid = osc::App::meshes().get100x100GridMesh();

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

    std::shared_ptr<experimental::Mesh const> cube = App::meshes().getCubeWireMesh();
    DrawBVHRecursive(cube, sceneBVH, 0, out);
}

void osc::DrawAABB(AABB const& aabb, std::vector<SceneDecoration>& out)
{
    std::shared_ptr<experimental::Mesh const> cube = App::meshes().getCubeWireMesh();
    glm::vec4 color = {0.0f, 0.0f, 0.0f, 1.0f};

    Transform t;
    t.scale = 0.5f * Dimensions(aabb);
    t.position = Midpoint(aabb);

    out.emplace_back(cube, t, color);
}

void osc::DrawAABBs(nonstd::span<AABB const> aabbs, std::vector<SceneDecoration>& out)
{
    std::shared_ptr<experimental::Mesh const> cube = App::meshes().getCubeWireMesh();
    glm::vec4 color = {0.0f, 0.0f, 0.0f, 1.0f};

    for (AABB const& aabb : aabbs)
    {
        Transform t;
        t.scale = 0.5f * Dimensions(aabb);
        t.position = Midpoint(aabb);

        out.emplace_back(cube, t, color);
    }
}

void osc::DrawXZFloorLines(std::vector<SceneDecoration>& out)
{
    std::shared_ptr<experimental::Mesh const> yLine = App::meshes().getYLineMesh();

    // X line
    {
        glm::vec4 const color = {1.0f, 0.0f, 0.0f, 1.0f};

        Transform t;
        t.rotation = glm::angleAxis(fpi2, glm::vec3{0.0f, 0.0f, 1.0f});

        out.emplace_back(yLine, t, color);
    }

    // Z line
    {
        glm::vec4 const color = {0.0f, 1.0f, 0.0f, 1.0f};

        Transform t;
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

    BVH_BuildFromAABBs(bvh, aabbs.data(), aabbs.size());
}

// returns all collisions along a ray
std::vector<osc::SceneCollision> osc::GetAllSceneCollisions(BVH const& bvh, nonstd::span<SceneDecoration const> decorations, Line const& ray)
{
    // use scene BVH to intersect the ray with the scene
    std::vector<BVHCollision> sceneCollisions;
    BVH_GetRayAABBCollisions(bvh, ray, &sceneCollisions);

    // perform ray-triangle intersections tests on the scene hits
    std::vector<SceneCollision> rv;
    for (BVHCollision const& c : sceneCollisions)
    {
        SceneDecoration const& decoration = decorations[c.primId];
        RayCollision const maybeCollision = GetClosestWorldspaceRayCollision(*decoration.mesh, decoration.transform, ray);

        if (maybeCollision)
        {
            rv.emplace_back(ray.origin + maybeCollision.distance * ray.dir, static_cast<size_t>(c.primId), maybeCollision.distance);
        }
    }
    return rv;
}

osc::RayCollision osc::GetClosestWorldspaceRayCollision(experimental::Mesh const& mesh, Transform const& transform, Line const& worldspaceRay)
{
    RayCollision rv{false, 0.0f};

    if (mesh.getTopography() != osc::experimental::MeshTopography::Triangles)
    {
        return rv;
    }

    Line const modelspaceRay = TransformLine(worldspaceRay, ToInverseMat4(transform));

    // TODO: this is going to be hellishly slow
    std::vector<uint32_t> const indices = mesh.getIndices();

    BVHCollision collision;
    if (BVH_GetClosestRayIndexedTriangleCollision(mesh.getBVH(), mesh.getVerts(), indices, modelspaceRay, &collision))
    {
        glm::vec3 const locationModelspace = modelspaceRay.origin + collision.distance * modelspaceRay.dir;
        glm::vec3 const locationWorldspace = transform * locationModelspace;

        rv.hit = true;
        rv.distance = glm::length(locationWorldspace - worldspaceRay.origin);
    }

    return rv;
}