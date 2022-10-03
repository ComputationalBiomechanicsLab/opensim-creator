#include "GraphicsHelpers.hpp"

#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
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
        std::shared_ptr<osc::Mesh const> const& mesh,
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
        std::shared_ptr<osc::Mesh const> grid = osc::App::meshes().get100x100GridMesh();

        osc::Transform t;
        t.scale *= glm::vec3{50.0f, 50.0f, 1.0f};
        t.rotation = rotation;

        glm::vec4 const color = {0.7f, 0.7f, 0.7f, 0.15f};

        out.emplace_back(grid, t, color);
    }


    struct Tetrahedron final {

        glm::vec3 Verts[4];

        glm::vec3 const& operator[](size_t i) const
        {
            return Verts[i];
        }
        glm::vec3& operator[](size_t i)
        {
            return Verts[i];
        }
        constexpr size_t size() const
        {
            return 4;
        }
    };

    // returns the volume of a given tetrahedron, defined as 4 points in space
    double Volume(Tetrahedron const& t)
    {
        // sources:
        //
        // http://forums.cgsociety.org/t/how-to-calculate-center-of-mass-for-triangular-mesh/1309966
        // https://stackoverflow.com/questions/9866452/calculate-volume-of-any-tetrahedron-given-4-points

        glm::mat<4, 4, double> m
        {
            glm::vec<4, double>{t[0], 1.0},
            glm::vec<4, double>{t[1], 1.0},
            glm::vec<4, double>{t[2], 1.0},
            glm::vec<4, double>{t[3], 1.0},
        };

        return glm::determinant(m) / 6.0;
    }

    // returns spatial centerpoint of a given tetrahedron
    glm::vec<3, double> Center(Tetrahedron const& t)
    {
        // arithmetic mean of tetrahedron vertices

        glm::vec<3, double> acc = t[0];
        for (size_t i = 1; i < t.size(); ++i)
        {
            acc += t[i];
        }
        acc /= static_cast<double>(t.size());
        return acc;
    }
}

void osc::DrawBVH(BVH const& sceneBVH, std::vector<SceneDecoration>& out)
{
    if (sceneBVH.nodes.empty())
    {
        return;
    }

    std::shared_ptr<Mesh const> cube = App::meshes().getCubeWireMesh();
    DrawBVHRecursive(cube, sceneBVH, 0, out);
}

void osc::DrawAABB(AABB const& aabb, std::vector<SceneDecoration>& out)
{
    std::shared_ptr<Mesh const> cube = App::meshes().getCubeWireMesh();
    glm::vec4 color = {0.0f, 0.0f, 0.0f, 1.0f};

    Transform t;
    t.scale = 0.5f * Dimensions(aabb);
    t.position = Midpoint(aabb);

    out.emplace_back(cube, t, color);
}

void osc::DrawAABBs(nonstd::span<AABB const> aabbs, std::vector<SceneDecoration>& out)
{
    std::shared_ptr<Mesh const> cube = App::meshes().getCubeWireMesh();
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
    std::shared_ptr<Mesh const> yLine = App::meshes().getYLineMesh();

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

osc::RayCollision osc::GetClosestWorldspaceRayCollision(Mesh const& mesh, Transform const& transform, Line const& worldspaceRay)
{
    RayCollision rv{false, 0.0f};

    if (mesh.getTopography() != MeshTopography::Triangles)
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
    std::vector<uint32_t> const indices = m.getIndices();
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
    std::vector<uint32_t> indices = m.getIndices();
    nonstd::span<glm::vec3 const> const verts = m.getVerts();

    glm::vec3 acc = {0.0f, 0.0f, 0.0f};
    for (uint32_t index : indices)
    {
        acc += verts[index];
    }
    acc /= static_cast<float>(verts.size());

    return acc;
}