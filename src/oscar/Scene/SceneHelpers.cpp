#include "SceneHelpers.hpp"

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshIndicesView.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/CollisionTests.hpp>
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
#include <oscar/Utils/At.hpp>

#include <filesystem>
#include <functional>
#include <numbers>
#include <optional>
#include <vector>

using namespace osc::literals;
using osc::Quat;
using osc::SceneCache;
using osc::SceneDecoration;
using osc::Vec3;

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
    float const len = Length(startToEnd);
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

    // then perform a ray-AABB (of triangles) collision
    std::optional<RayCollision> rv;
    triangleBVH.forEachRayAABBCollision(modelspaceRay, [&mesh, &transform, &worldspaceRay, &modelspaceRay, &rv](BVHCollision bvhCollision)
    {
        // then perform a ray-triangle collision
        if (auto triangleCollision = GetRayCollisionTriangle(modelspaceRay, mesh.getTriangleAt(bvhCollision.id)))
        {
            // map it back into worldspace and check if it's closer
            Vec3 const locationWorldspace = transform * triangleCollision->position;
            float const distance = Length(locationWorldspace - worldspaceRay.origin);
            if (!rv || rv->distance > distance)
            {
                // if it's closer, update the return value
                rv = RayCollision{distance, locationWorldspace};
            }
        }
    });
    return rv;
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
        Identity<Transform>(),
        ray
    );
}

osc::SceneRendererParams osc::CalcStandardDarkSceneRenderParams(
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

osc::Material osc::CreateWireframeOverlayMaterial(
    AppConfig const& config,
    ShaderCache& cache)
{
    std::filesystem::path const vertShader = config.getResourcePath("oscar/shaders/SceneRenderer/SolidColor.vert");
    std::filesystem::path const fragShader = config.getResourcePath("oscar/shaders/SceneRenderer/SolidColor.frag");
    Material material{cache.load(vertShader, fragShader)};
    material.setColor("uDiffuseColor", {0.0f, 0.6f});
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
