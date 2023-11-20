#include "SceneHelpers.hpp"

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshIndicesView.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/Line.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Quat.hpp>
#include <oscar/Maths/RayCollision.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Segment.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>

#include <filesystem>
#include <functional>
#include <numbers>
#include <optional>
#include <vector>

using osc::Color;
using osc::Mesh;
using osc::Quat;
using osc::SceneCache;
using osc::SceneDecoration;
using osc::Transform;
using osc::Vec3;

namespace
{
    void DrawGrid(
        SceneCache& cache,
        Quat const& rotation,
        std::function<void(SceneDecoration&&)> const& out)
    {
        Mesh const grid = cache.get100x100GridMesh();

        Transform t;
        t.scale *= Vec3{50.0f, 50.0f, 1.0f};
        t.rotation = rotation;

        Color const color = {0.7f, 0.7f, 0.7f, 0.15f};

        out(SceneDecoration{grid, t, color});
    }
}

void osc::DrawBVH(
    SceneCache& cache,
    BVH const& sceneBVH,
    std::function<void(SceneDecoration&&)> const& out)
{
    sceneBVH.forEachLeafOrInnerNodeUnordered([cube = cache.getCubeWireMesh(), &out](BVHNode const& node)
    {
        Transform t;
        t.scale *= 0.5f * Dimensions(node.getBounds());
        t.position = Midpoint(node.getBounds());
        out(SceneDecoration{cube, t, Color::black()});
    });
}

void osc::DrawAABB(
    SceneCache& cache,
    AABB const& aabb,
    std::function<void(SceneDecoration&&)> const& out)
{
    Mesh const cube = cache.getCubeWireMesh();

    Transform t;
    t.scale = 0.5f * Dimensions(aabb);
    t.position = Midpoint(aabb);

    out(SceneDecoration{cube, t, Color::black()});
}

void osc::DrawAABBs(
    SceneCache& cache,
    std::span<AABB const> aabbs,
    std::function<void(SceneDecoration&&)> const& out)
{
    Mesh const cube = cache.getCubeWireMesh();

    for (AABB const& aabb : aabbs)
    {
        Transform t;
        t.scale = 0.5f * Dimensions(aabb);
        t.position = Midpoint(aabb);

        out(SceneDecoration{cube, t, Color::black()});
    }
}

void osc::DrawBVHLeafNodes(
    SceneCache& cache,
    BVH const& bvh,
    std::function<void(SceneDecoration&&)> const& out)
{
    bvh.forEachLeafNode([&cache, &out](BVHNode const& node)
    {
        DrawAABB(cache, node.getBounds(), out);
    });
}

void osc::DrawXZFloorLines(
    SceneCache& cache,
    std::function<void(SceneDecoration&&)> const& out,
    float scale)
{
    Mesh const yLine = cache.getYLineMesh();

    // X line
    {
        Transform t;
        t.scale *= scale;
        t.rotation = AngleAxis(std::numbers::pi_v<float>/2.0f, Vec3{0.0f, 0.0f, 1.0f});

        out(SceneDecoration{yLine, t, Color::red()});
    }

    // Z line
    {
        Transform t;
        t.scale *= scale;
        t.rotation = AngleAxis(std::numbers::pi_v<float>/2.0f, Vec3{1.0f, 0.0f, 0.0f});

        out(SceneDecoration{yLine, t, Color::blue()});
    }
}

void osc::DrawXZGrid(
    SceneCache& cache,
    std::function<void(SceneDecoration&&)> const& out)
{
    Quat const rotation = AngleAxis(std::numbers::pi_v<float>/2.0f, Vec3{1.0f, 0.0f, 0.0f});
    DrawGrid(cache, rotation, out);
}

void osc::DrawXYGrid(
    SceneCache& cache,
    std::function<void(SceneDecoration&&)> const& out)
{
    auto const rotation = Identity<Quat>();
    DrawGrid(cache, rotation, out);
}

void osc::DrawYZGrid(
    SceneCache& cache,
    std::function<void(SceneDecoration&&)> const& out)
{
    Quat const rotation = AngleAxis(std::numbers::pi_v<float>/2.0f, Vec3{0.0f, 1.0f, 0.0f});
    DrawGrid(cache, rotation, out);
}

osc::ArrowProperties::ArrowProperties() :
    worldspaceStart{},
    worldspaceEnd{},
    tipLength{},
    neckThickness{},
    headThickness{},
    color{Color::black()}
{
}

void osc::DrawArrow(
    SceneCache& cache,
    ArrowProperties const& props,
    std::function<void(SceneDecoration&&)> const& out)
{
    Vec3 startToEnd = props.worldspaceEnd - props.worldspaceStart;
    float const len = Length(startToEnd);
    Vec3 const direction = startToEnd/len;

    Vec3 const neckStart = props.worldspaceStart;
    Vec3 const neckEnd = props.worldspaceStart + (len - props.tipLength)*direction;
    Vec3 const headStart = neckEnd;
    Vec3 const headEnd = props.worldspaceEnd;

    // emit neck cylinder
    Transform const neckXform = YToYCylinderToSegmentTransform({neckStart, neckEnd}, props.neckThickness);
    out(SceneDecoration{cache.getCylinderMesh(), neckXform, props.color});

    // emit head cone
    Transform const headXform = YToYCylinderToSegmentTransform({headStart, headEnd}, props.headThickness);
    out(SceneDecoration{cache.getConeMesh(), headXform, props.color});
}

void osc::DrawLineSegment(
    SceneCache& cache,
    Segment const& segment,
    Color const& color,
    float radius,
    std::function<void(SceneDecoration&&)> const& out)
{
    Transform const cylinderXform = YToYCylinderToSegmentTransform(segment, radius);
    out(SceneDecoration{cache.getCylinderMesh(), cylinderXform, color});
}

osc::AABB osc::GetWorldspaceAABB(SceneDecoration const& cd)
{
    return TransformAABB(cd.mesh.getBounds(), cd.transform);
}

void osc::UpdateSceneBVH(std::span<SceneDecoration const> sceneEls, BVH& bvh)
{
    std::vector<AABB> aabbs;
    aabbs.reserve(sceneEls.size());
    for (SceneDecoration const& el : sceneEls)
    {
        aabbs.push_back(GetWorldspaceAABB(el));
    }

    bvh.buildFromAABBs(aabbs);
}

std::vector<osc::SceneCollision> osc::GetAllSceneCollisions(
    BVH const& bvh,
    SceneCache& sceneCache,
    std::span<SceneDecoration const> decorations,
    Line const& ray)
{
    // use scene BVH to intersect the ray with the scene
    std::vector<BVHCollision> const sceneCollisions = bvh.getRayAABBCollisions(ray);

    // perform ray-triangle intersections tests on the scene hits
    std::vector<SceneCollision> rv;
    for (BVHCollision const& c : sceneCollisions)
    {
        SceneDecoration const& decoration = decorations[c.id];
        BVH const& decorationBVH = sceneCache.getBVH(decoration.mesh);
        std::optional<RayCollision> const maybeCollision = GetClosestWorldspaceRayCollision(
            decoration.mesh,
            decorationBVH,
            decoration.transform,
            ray
        );

        if (maybeCollision)
        {
            rv.emplace_back(decoration.id, static_cast<size_t>(c.id), maybeCollision->position, maybeCollision->distance);
        }
    }
    return rv;
}

std::optional<osc::RayCollision> osc::GetClosestWorldspaceRayCollision(
    Mesh const& mesh,
    BVH const& triangleBVH,
    Transform const& transform,
    Line const& worldspaceRay)
{
    if (mesh.getTopology() != MeshTopology::Triangles)
    {
        return std::nullopt;
    }

    // map the ray into the mesh's modelspace, so that we compute a ray-mesh collision
    Line const modelspaceRay = InverseTransformLine(worldspaceRay, transform);

    MeshIndicesView const indices = mesh.getIndices();
    std::optional<BVHCollision> const maybeCollision = indices.isU16() ?
        triangleBVH.getClosestRayIndexedTriangleCollision(mesh.getVerts(), indices.toU16Span(), modelspaceRay) :
        triangleBVH.getClosestRayIndexedTriangleCollision(mesh.getVerts(), indices.toU32Span(), modelspaceRay);

    if (maybeCollision)
    {
        // map the ray back into worldspace
        Vec3 const locationWorldspace = transform * maybeCollision->position;
        float const distance = Length(locationWorldspace - worldspaceRay.origin);
        return RayCollision{.distance = distance, .position = locationWorldspace};
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<osc::RayCollision> osc::GetClosestWorldspaceRayCollision(
    PolarPerspectiveCamera const& camera,
    Mesh const& mesh,
    BVH const& triangleBVH,
    Rect const& renderScreenRect,
    Vec2 mouseScreenPos)
{
    Line const ray = camera.unprojectTopLeftPosToWorldRay(
        mouseScreenPos - renderScreenRect.p1,
        Dimensions(renderScreenRect)
    );

    return osc::GetClosestWorldspaceRayCollision(
        mesh,
        triangleBVH,
        Transform{},
        ray
    );
}

osc::SceneRendererParams osc::CalcStandardDarkSceneRenderParams(
    PolarPerspectiveCamera const& camera,
    AntiAliasingLevel antiAliasingLevel,
    Vec2 renderDims)
{
    osc::SceneRendererParams rv;
    rv.dimensions = renderDims;
    rv.antiAliasingLevel = antiAliasingLevel;
    rv.drawMeshNormals = false;
    rv.drawFloor = false;
    rv.viewMatrix = camera.getViewMtx();
    rv.projectionMatrix = camera.getProjMtx(AspectRatio(renderDims));
    rv.viewPos = camera.getPos();
    rv.lightDirection = RecommendedLightDirection(camera);
    rv.backgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};
    return rv;
}

osc::Material osc::CreateWireframeOverlayMaterial(
    AppConfig const& config,
    ShaderCache& cache)
{
    std::filesystem::path const vertShader = config.getResourceDir() / "oscar/shaders/SceneRenderer/SolidColor.vert";
    std::filesystem::path const fragShader = config.getResourceDir() / "oscar/shaders/SceneRenderer/SolidColor.frag";
    Material material{cache.load(vertShader, fragShader)};
    material.setColor("uDiffuseColor", {0.0f, 0.0f, 0.0f, 0.6f});
    material.setWireframeMode(true);
    material.setTransparent(true);
    return material;
}

osc::BVH osc::CreateTriangleBVHFromMesh(Mesh const& mesh)
{
    BVH rv;

    auto const indices = mesh.getIndices();

    if (indices.empty())
    {
        return rv;
    }
    else if (mesh.getTopology() != MeshTopology::Triangles)
    {
        return rv;
    }
    else if (indices.isU32())
    {
        rv.buildFromIndexedTriangles(mesh.getVerts() , indices.toU32Span());
    }
    else
    {
        rv.buildFromIndexedTriangles(mesh.getVerts(), indices.toU16Span());
    }

    return rv;
}
