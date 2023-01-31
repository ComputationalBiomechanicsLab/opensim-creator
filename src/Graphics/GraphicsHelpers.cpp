#include "GraphicsHelpers.hpp"

#include "src/Graphics/Image.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/Segment.hpp"
#include "src/Maths/Tetrahedron.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>

namespace
{
    // assumes `pos` is in-bounds
    void DrawBVHRecursive(
        osc::Mesh const& mesh,
        osc::BVH const& bvh,
        ptrdiff_t pos,
        std::function<void(osc::SceneDecoration&&)> const& out)
    {
        osc::BVHNode const& n = bvh.nodes[pos];

        osc::Transform t;
        t.scale *= 0.5f * Dimensions(n.getBounds());
        t.position = Midpoint(n.getBounds());

        glm::vec4 const color = {0.0f, 0.0f, 0.0f, 1.0f};

        out(osc::SceneDecoration{mesh, t, color});

        if (n.isNode())
        {
            // it's an internal node
            DrawBVHRecursive(mesh, bvh, pos+1, out);
            DrawBVHRecursive(mesh, bvh, pos+n.getNumLhsNodes()+1, out);
        }
    }

    void DrawGrid(
        osc::MeshCache& cache,
        glm::quat const& rotation,
        std::function<void(osc::SceneDecoration&&)> const& out)
    {
        osc::Mesh const grid = cache.get100x100GridMesh();

        osc::Transform t;
        t.scale *= glm::vec3{50.0f, 50.0f, 1.0f};
        t.rotation = rotation;

        glm::vec4 const color = {0.7f, 0.7f, 0.7f, 0.15f};

        out(osc::SceneDecoration{grid, t, color});
    }
}

void osc::DrawBVH(
    MeshCache& cache,
    BVH const& sceneBVH,
    std::function<void(osc::SceneDecoration&&)> const& out)
{
    if (sceneBVH.nodes.empty())
    {
        return;
    }

    Mesh const cube = cache.getCubeWireMesh();
    DrawBVHRecursive(cube, sceneBVH, 0, out);
}

void osc::DrawAABB(
    MeshCache& cache,
    AABB const& aabb,
    std::function<void(osc::SceneDecoration&&)> const& out)
{
    Mesh const cube = cache.getCubeWireMesh();
    glm::vec4 const color = {0.0f, 0.0f, 0.0f, 1.0f};

    Transform t;
    t.scale = 0.5f * Dimensions(aabb);
    t.position = Midpoint(aabb);

    out(osc::SceneDecoration{cube, t, color});
}

void osc::DrawAABBs(
    MeshCache& cache,
    nonstd::span<AABB const> aabbs,
    std::function<void(osc::SceneDecoration&&)> const& out)
{
    Mesh const cube = cache.getCubeWireMesh();
    glm::vec4 const color = {0.0f, 0.0f, 0.0f, 1.0f};

    for (AABB const& aabb : aabbs)
    {
        Transform t;
        t.scale = 0.5f * Dimensions(aabb);
        t.position = Midpoint(aabb);

        out(osc::SceneDecoration{cube, t, color});
    }
}

void osc::DrawXZFloorLines(
    MeshCache& cache,
    std::function<void(osc::SceneDecoration&&)> const& out,
    float scale)
{
    Mesh const yLine = cache.getYLineMesh();

    // X line
    {
        glm::vec4 const color = {1.0f, 0.0f, 0.0f, 1.0f};

        Transform t;
        t.scale *= scale;
        t.rotation = glm::angleAxis(fpi2, glm::vec3{0.0f, 0.0f, 1.0f});

        out(osc::SceneDecoration{yLine, t, color});
    }

    // Z line
    {
        glm::vec4 const color = {0.0f, 1.0f, 0.0f, 1.0f};

        Transform t;
        t.scale *= scale;
        t.rotation = glm::angleAxis(fpi2, glm::vec3{1.0f, 0.0f, 0.0f});

        out(osc::SceneDecoration{yLine, t, color});
    }
}

void osc::DrawXZGrid(
    MeshCache& cache,
    std::function<void(osc::SceneDecoration&&)> const& out)
{
    glm::quat const rotation = glm::angleAxis(fpi2, glm::vec3{1.0f, 0.0f, 0.0f});
    DrawGrid(cache, rotation, out);
}

void osc::DrawXYGrid(
    MeshCache& cache,
    std::function<void(osc::SceneDecoration&&)> const& out)
{
    glm::quat const rotation = glm::identity<glm::quat>();
    DrawGrid(cache, rotation, out);
}

void osc::DrawYZGrid(
    MeshCache& cache,
    std::function<void(osc::SceneDecoration&&)> const& out)
{
    glm::quat rotation = glm::angleAxis(fpi2, glm::vec3{0.0f, 1.0f, 0.0f});
    DrawGrid(cache, rotation, out);
}

osc::ArrowProperties::ArrowProperties() :
    worldspaceStart{},
    worldspaceEnd{},
    tipLength{},
    neckThickness{},
    headThickness{},
    color{}
{
}

void osc::DrawArrow(
    MeshCache& cache,
    ArrowProperties const& props,
    std::function<void(osc::SceneDecoration&&)> const& out)
{
    glm::vec3 startToEnd = props.worldspaceEnd - props.worldspaceStart;
    float const len = glm::length(startToEnd);
    glm::vec3 const dir = startToEnd/len;

    glm::vec3 const neckStart = props.worldspaceStart;
    glm::vec3 const neckEnd = props.worldspaceStart + (len - props.tipLength)*dir;
    glm::vec3 const headStart = neckEnd;
    glm::vec3 const headEnd = props.worldspaceEnd;

    // emit neck cylinder
    Transform const neckXform = SimbodyCylinderToSegmentTransform({neckStart, neckEnd}, props.neckThickness);
    out(osc::SceneDecoration{cache.getCylinderMesh(), neckXform, props.color});

    // emit head cone
    Transform const headXform = SimbodyCylinderToSegmentTransform({headStart, headEnd}, props.headThickness);
    out(osc::SceneDecoration{cache.getConeMesh(), headXform, props.color});
}

void osc::DrawLineSegment(
    MeshCache& cache,
    Segment const& segment,
    glm::vec4 const& color,
    float radius,
    std::function<void(osc::SceneDecoration&&)> const& out)
{
    Transform const cylinderXform = SimbodyCylinderToSegmentTransform(segment, radius);
    out(osc::SceneDecoration{cache.getCylinderMesh(), cylinderXform, color});
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
std::vector<osc::SceneCollision> osc::GetAllSceneCollisions(
    BVH const& bvh,
    nonstd::span<SceneDecoration const> decorations,
    Line const& ray)
{
    // use scene BVH to intersect the ray with the scene
    std::vector<BVHCollision> const sceneCollisions = bvh.getRayAABBCollisions(ray);

    // perform ray-triangle intersections tests on the scene hits
    std::vector<SceneCollision> rv;
    for (BVHCollision const& c : sceneCollisions)
    {
        SceneDecoration const& decoration = decorations[c.id];
        std::optional<RayCollision> const maybeCollision = GetClosestWorldspaceRayCollision(decoration.mesh, decoration.transform, ray);

        if (maybeCollision)
        {
            rv.emplace_back(decoration.id, static_cast<size_t>(c.id), maybeCollision->position, maybeCollision->distance);
        }
    }
    return rv;
}

std::optional<osc::RayCollision> osc::GetClosestWorldspaceRayCollision(Mesh const& mesh, Transform const& transform, Line const& worldspaceRay)
{
    if (mesh.getTopology() != MeshTopology::Triangles)
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

std::optional<osc::RayCollision> osc::GetClosestWorldspaceRayCollision(
    PolarPerspectiveCamera const& camera,
    Mesh const& mesh,
    Rect const& renderScreenRect,
    glm::vec2 mouseScreenPos)
{
    osc::Line const ray = camera.unprojectTopLeftPosToWorldRay(
        mouseScreenPos - renderScreenRect.p1,
        osc::Dimensions(renderScreenRect)
    );

    return osc::GetClosestWorldspaceRayCollision(
        mesh,
        osc::Transform{},
        ray
    );
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

    if (m.getTopology() != osc::MeshTopology::Triangles)
    {
        return {0.0f, 0.0f, 0.0f};
    }

    nonstd::span<glm::vec3 const> const verts = m.getVerts();
    MeshIndicesView const indices = m.getIndices();
    size_t const len = (indices.size() / 3) * 3;  // paranioa

    double totalVolume = 0.0f;
    glm::dvec3 weightedCenterOfMass = {0.0, 0.0, 0.0};
    for (size_t i = 0; i < len; i += 3)
    {
        Tetrahedron tetrahedron;
        tetrahedron[0] = {0.0f, 0.0f, 0.0f};  // reference point
        tetrahedron[1] = verts[indices[i]];
        tetrahedron[2] = verts[indices[i+1]];
        tetrahedron[3] = verts[indices[i+2]];

        double const volume = Volume(tetrahedron);
        glm::dvec3 const centerOfMass = Center(tetrahedron);

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

osc::Material osc::CreateWireframeOverlayMaterial(Config const& config, ShaderCache& cache)
{
    std::filesystem::path const vertShader = config.getResourceDir() / "shaders/SceneSolidColor.vert";
    std::filesystem::path const fragShader = config.getResourceDir() / "shaders/SceneSolidColor.frag";
    osc::Material material{cache.load(vertShader, fragShader)};
    material.setVec4("uDiffuseColor", {0.0f, 0.0f, 0.0f, 0.6f});
    material.setWireframeMode(true);
    material.setTransparent(true);
    return material;
}

osc::Texture2D osc::LoadTexture2DFromImage(std::filesystem::path const& path, ImageFlags flags)
{
    Image const img = LoadImage(path, flags);
    return Texture2D{img.getDimensions(), img.getPixelData(), img.getNumChannels()};
}

osc::AABB osc::GetWorldspaceAABB(SceneDecoration const& cd)
{
    return TransformAABB(cd.mesh.getBounds(), cd.transform);
}

osc::SceneRendererParams osc::CalcStandardDarkSceneRenderParams(
    PolarPerspectiveCamera const& camera,
    glm::vec2 renderDims)
{
    osc::SceneRendererParams rv;
    rv.drawFloor = false;
    rv.backgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};
    rv.dimensions = renderDims;
    rv.viewMatrix = camera.getViewMtx();
    rv.projectionMatrix = camera.getProjMtx(osc::AspectRatio(renderDims));
    rv.samples = osc::App::get().getMSXAASamplesRecommended();
    rv.lightDirection = osc::RecommendedLightDirection(camera);
    return rv;
}