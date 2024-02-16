#include "SceneHelpers.h"

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshIndicesView.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Graphics/Scene/ShaderCache.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/CollisionTests.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/RayCollision.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Segment.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/At.h>

#include <functional>
#include <optional>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    void DrawGrid(
        SceneCache& cache,
        Quat const& rotation,
        std::function<void(SceneDecoration&&)> const& out)
    {
        out({
            .mesh = cache.get100x100GridMesh(),
            .transform = {
                .scale = Vec3{50.0f, 50.0f, 1.0f},
                .rotation = rotation,
            },
            .color = {0.7f, 0.15f},
        });
    }
}

void osc::DrawBVH(
    SceneCache& cache,
    BVH const& sceneBVH,
    std::function<void(SceneDecoration&&)> const& out)
{
    sceneBVH.forEachLeafOrInnerNodeUnordered([cube = cache.getCubeWireMesh(), &out](BVHNode const& node)
    {
        out({
            .mesh = cube,
            .transform = {
                .scale = HalfWidths(node.getBounds()),
                .position = Midpoint(node.getBounds()),
            },
            .color = Color::black(),
        });
    });
}

void osc::DrawAABB(
    SceneCache& cache,
    AABB const& aabb,
    std::function<void(SceneDecoration&&)> const& out)
{
    DrawAABBs(cache, {{aabb}}, out);
}

void osc::DrawAABBs(
    SceneCache& cache,
    std::span<AABB const> aabbs,
    std::function<void(SceneDecoration&&)> const& out)
{
    Mesh const cube = cache.getCubeWireMesh();
    for (AABB const& aabb : aabbs)
    {
        out({
            .mesh = cube,
            .transform = {
                .scale = HalfWidths(aabb),
                .position = Midpoint(aabb),
            },
            .color = Color::black(),
        });
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
    out({
        .mesh = yLine,
        .transform = {
            .scale = Vec3{scale},
            .rotation = AngleAxis(90_deg, Vec3{0.0f, 0.0f, 1.0f}),
        },
        .color = Color::red(),
    });

    // Z line
    out({
        .mesh = yLine,
        .transform = {
            .scale = Vec3{scale},
            .rotation = AngleAxis(90_deg, Vec3{1.0f, 0.0f, 0.0f}),
        },
        .color = Color::blue(),
    });
}

void osc::DrawXZGrid(
    SceneCache& cache,
    std::function<void(SceneDecoration&&)> const& out)
{
    Quat const rotation = AngleAxis(90_deg, Vec3{1.0f, 0.0f, 0.0f});
    DrawGrid(cache, rotation, out);
}

void osc::DrawXYGrid(
    SceneCache& cache,
    std::function<void(SceneDecoration&&)> const& out)
{
    DrawGrid(cache, Identity<Quat>(), out);
}

void osc::DrawYZGrid(
    SceneCache& cache,
    std::function<void(SceneDecoration&&)> const& out)
{
    Quat const rotation = AngleAxis(90_deg, Vec3{0.0f, 1.0f, 0.0f});
    DrawGrid(cache, rotation, out);
}

void osc::DrawArrow(
    SceneCache& cache,
    ArrowProperties const& props,
    std::function<void(SceneDecoration&&)> const& out)
{
    Vec3 startToEnd = props.worldspaceEnd - props.worldspaceStart;
    float const len = length(startToEnd);
    Vec3 const direction = startToEnd/len;

    Vec3 const neckStart = props.worldspaceStart;
    Vec3 const neckEnd = props.worldspaceStart + (len - props.tipLength)*direction;
    Vec3 const headStart = neckEnd;
    Vec3 const headEnd = props.worldspaceEnd;

    // emit neck cylinder
    out({
        .mesh = cache.getCylinderMesh(),
        .transform = YToYCylinderToSegmentTransform({neckStart, neckEnd}, props.neckThickness),
        .color = props.color
    });

    // emit head cone
    out({
        .mesh = cache.getConeMesh(),
        .transform = YToYCylinderToSegmentTransform({headStart, headEnd}, props.headThickness),
        .color = props.color
    });
}

void osc::DrawLineSegment(
    SceneCache& cache,
    Segment const& segment,
    Color const& color,
    float radius,
    std::function<void(SceneDecoration&&)> const& out)
{
    out({
        .mesh = cache.getCylinderMesh(),
        .transform = YToYCylinderToSegmentTransform(segment, radius),
        .color = color,
    });
}

AABB osc::GetWorldspaceAABB(SceneDecoration const& cd)
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

std::vector<SceneCollision> osc::GetAllSceneCollisions(
    BVH const& bvh,
    SceneCache& sceneCache,
    std::span<SceneDecoration const> decorations,
    Line const& ray)
{
    std::vector<SceneCollision> rv;
    bvh.forEachRayAABBCollision(ray, [&sceneCache, &decorations, &ray, &rv](BVHCollision sceneCollision)
    {
        // perform ray-triangle intersection tests on the scene collisions
        SceneDecoration const& decoration = At(decorations, sceneCollision.id);
        BVH const& decorationBVH = sceneCache.getBVH(decoration.mesh);

        std::optional<RayCollision> const maybeCollision = GetClosestWorldspaceRayCollision(
            decoration.mesh,
            decorationBVH,
            decoration.transform,
            ray
        );

        if (maybeCollision)
        {
            rv.push_back({
                .decorationID = decoration.id,
                .decorationIndex = static_cast<size_t>(sceneCollision.id),
                .worldspaceLocation = maybeCollision->position,
                .distanceFromRayOrigin = maybeCollision->distance,
            });
        }
    });
    return rv;
}

std::optional<RayCollision> osc::GetClosestWorldspaceRayCollision(
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

    // then perform a ray-AABB (of triangles) collision
    std::optional<RayCollision> rv;
    triangleBVH.forEachRayAABBCollision(modelspaceRay, [&mesh, &transform, &worldspaceRay, &modelspaceRay, &rv](BVHCollision bvhCollision)
    {
        // then perform a ray-triangle collision
        if (auto triangleCollision = GetRayCollisionTriangle(modelspaceRay, mesh.getTriangleAt(bvhCollision.id)))
        {
            // map it back into worldspace and check if it's closer
            Vec3 const locationWorldspace = transform * triangleCollision->position;
            float const distance = length(locationWorldspace - worldspaceRay.origin);
            if (!rv || rv->distance > distance)
            {
                // if it's closer, update the return value
                rv = RayCollision{distance, locationWorldspace};
            }
        }
    });
    return rv;
}

std::optional<RayCollision> osc::GetClosestWorldspaceRayCollision(
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

    return GetClosestWorldspaceRayCollision(
        mesh,
        triangleBVH,
        Identity<Transform>(),
        ray
    );
}

SceneRendererParams osc::CalcStandardDarkSceneRenderParams(
    PolarPerspectiveCamera const& camera,
    AntiAliasingLevel antiAliasingLevel,
    Vec2 renderDims)
{
    SceneRendererParams rv;
    rv.dimensions = renderDims;
    rv.antiAliasingLevel = antiAliasingLevel;
    rv.drawMeshNormals = false;
    rv.drawFloor = false;
    rv.viewMatrix = camera.getViewMtx();
    rv.projectionMatrix = camera.getProjMtx(AspectRatio(renderDims));
    rv.viewPos = camera.getPos();
    rv.lightDirection = RecommendedLightDirection(camera);
    rv.backgroundColor = {0.1f, 1.0f};
    return rv;
}

Material osc::CreateWireframeOverlayMaterial(
    ShaderCache& cache)
{
    Material material{cache.load("oscar/shaders/SceneRenderer/SolidColor.vert", "oscar/shaders/SceneRenderer/SolidColor.frag")};
    material.setColor("uDiffuseColor", {0.0f, 0.6f});
    material.setWireframeMode(true);
    material.setTransparent(true);
    return material;
}

BVH osc::CreateTriangleBVHFromMesh(Mesh const& mesh)
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
