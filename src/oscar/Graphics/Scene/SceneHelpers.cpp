#include "SceneHelpers.h"

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshIndicesView.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/CollisionTests.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/LineSegment.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PlaneFunctions.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/RayCollision.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/TrigonometricFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/Algorithms.h>

#include <functional>
#include <optional>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    void drawGrid(
        SceneCache& cache,
        const Quat& rotation,
        const std::function<void(SceneDecoration&&)>& out)
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

void osc::drawBVH(
    SceneCache& cache,
    const BVH& sceneBVH,
    const std::function<void(SceneDecoration&&)>& out)
{
    sceneBVH.forEachLeafOrInnerNodeUnordered([cube = cache.getCubeWireMesh(), &out](const BVHNode& node)
    {
        out({
            .mesh = cube,
            .transform = {
                .scale = half_widths(node.getBounds()),
                .position = centroid(node.getBounds()),
            },
            .color = Color::black(),
        });
    });
}

void osc::drawAABB(
    SceneCache& cache,
    const AABB& aabb,
    const std::function<void(SceneDecoration&&)>& out)
{
    drawAABBs(cache, {{aabb}}, out);
}

void osc::drawAABBs(
    SceneCache& cache,
    std::span<const AABB> aabbs,
    const std::function<void(SceneDecoration&&)>& out)
{
    const Mesh cube = cache.getCubeWireMesh();
    for (const AABB& aabb : aabbs) {
        out({
            .mesh = cube,
            .transform = {
                .scale = half_widths(aabb),
                .position = centroid(aabb),
            },
            .color = Color::black(),
        });
    }
}

void osc::drawBVHLeafNodes(
    SceneCache& cache,
    const BVH& bvh,
    const std::function<void(SceneDecoration&&)>& out)
{
    bvh.forEachLeafNode([&cache, &out](const BVHNode& node)
    {
        drawAABB(cache, node.getBounds(), out);
    });
}

void osc::drawXZFloorLines(
    SceneCache& cache,
    const std::function<void(SceneDecoration&&)>& out,
    float scale)
{
    const Mesh yLine = cache.getYLineMesh();

    // X line
    out({
        .mesh = yLine,
        .transform = {
            .scale = Vec3{scale},
            .rotation = angle_axis(90_deg, Vec3{0.0f, 0.0f, 1.0f}),
        },
        .color = Color::red(),
    });

    // Z line
    out({
        .mesh = yLine,
        .transform = {
            .scale = Vec3{scale},
            .rotation = angle_axis(90_deg, Vec3{1.0f, 0.0f, 0.0f}),
        },
        .color = Color::blue(),
    });
}

void osc::drawXZGrid(
    SceneCache& cache,
    const std::function<void(SceneDecoration&&)>& out)
{
    const Quat rotation = angle_axis(90_deg, Vec3{1.0f, 0.0f, 0.0f});
    drawGrid(cache, rotation, out);
}

void osc::drawXYGrid(
    SceneCache& cache,
    const std::function<void(SceneDecoration&&)>& out)
{
    drawGrid(cache, identity<Quat>(), out);
}

void osc::drawYZGrid(
    SceneCache& cache,
    const std::function<void(SceneDecoration&&)>& out)
{
    const Quat rotation = angle_axis(90_deg, Vec3{0.0f, 1.0f, 0.0f});
    drawGrid(cache, rotation, out);
}

void osc::drawArrow(
    SceneCache& cache,
    const ArrowProperties& props,
    const std::function<void(SceneDecoration&&)>& out)
{
    const Vec3 startToEnd = props.worldspaceEnd - props.worldspaceStart;
    const float len = length(startToEnd);
    const Vec3 direction = startToEnd/len;

    const Vec3 neckStart = props.worldspaceStart;
    const Vec3 neckEnd = props.worldspaceStart + (len - props.tipLength)*direction;
    const Vec3 headStart = neckEnd;
    const Vec3 headEnd = props.worldspaceEnd;

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

void osc::drawLineSegment(
    SceneCache& cache,
    const LineSegment& segment,
    const Color& color,
    float radius,
    const std::function<void(SceneDecoration&&)>& out)
{
    out({
        .mesh = cache.getCylinderMesh(),
        .transform = YToYCylinderToSegmentTransform(segment, radius),
        .color = color,
    });
}

AABB osc::getWorldspaceAABB(const SceneDecoration& cd)
{
    return transform_aabb(cd.transform, cd.mesh.getBounds());
}

void osc::updateSceneBVH(std::span<const SceneDecoration> sceneEls, BVH& bvh)
{
    std::vector<AABB> aabbs;
    aabbs.reserve(sceneEls.size());
    for (const SceneDecoration& el : sceneEls) {
        aabbs.push_back(getWorldspaceAABB(el));
    }

    bvh.buildFromAABBs(aabbs);
}

std::vector<SceneCollision> osc::getAllSceneCollisions(
    const BVH& bvh,
    SceneCache& sceneCache,
    std::span<const SceneDecoration> decorations,
    const Line& ray)
{
    std::vector<SceneCollision> rv;
    bvh.forEachRayAABBCollision(ray, [&sceneCache, &decorations, &ray, &rv](BVHCollision sceneCollision)
    {
        // perform ray-triangle intersection tests on the scene collisions
        const SceneDecoration& decoration = at(decorations, sceneCollision.id);
        const BVH& decorationBVH = sceneCache.getBVH(decoration.mesh);

        const std::optional<RayCollision> maybeCollision = getClosestWorldspaceRayCollision(
            decoration.mesh,
            decorationBVH,
            decoration.transform,
            ray
        );

        if (maybeCollision) {
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

std::optional<RayCollision> osc::getClosestWorldspaceRayCollision(
    const Mesh& mesh,
    const BVH& triangleBVH,
    const Transform& transform,
    const Line& worldspaceRay)
{
    if (mesh.getTopology() != MeshTopology::Triangles) {
        return std::nullopt;
    }

    // map the ray into the mesh's modelspace, so that we compute a ray-mesh collision
    const Line modelspaceRay = InverseTransformLine(worldspaceRay, transform);

    // then perform a ray-AABB (of triangles) collision
    std::optional<RayCollision> rv;
    triangleBVH.forEachRayAABBCollision(modelspaceRay, [&mesh, &transform, &worldspaceRay, &modelspaceRay, &rv](BVHCollision bvhCollision)
    {
        // then perform a ray-triangle collision
        if (auto triangleCollision = find_collision(modelspaceRay, mesh.getTriangleAt(bvhCollision.id))) {
            // map it back into worldspace and check if it's closer
            const Vec3 locationWorldspace = transform * triangleCollision->position;
            const float distance = length(locationWorldspace - worldspaceRay.origin);

            if (not rv or rv->distance > distance) {
                // if it's closer, update the return value
                rv = RayCollision{distance, locationWorldspace};
            }
        }
    });
    return rv;
}

std::optional<RayCollision> osc::getClosestWorldspaceRayCollision(
    const PolarPerspectiveCamera& camera,
    const Mesh& mesh,
    const BVH& triangleBVH,
    const Rect& renderScreenRect,
    Vec2 mouseScreenPos)
{
    const Line ray = camera.unprojectTopLeftPosToWorldRay(
        mouseScreenPos - renderScreenRect.p1,
        dimensions(renderScreenRect)
    );

    return getClosestWorldspaceRayCollision(
        mesh,
        triangleBVH,
        identity<Transform>(),
        ray
    );
}

SceneRendererParams osc::calcStandardDarkSceneRenderParams(
    const PolarPerspectiveCamera& camera,
    AntiAliasingLevel antiAliasingLevel,
    Vec2 renderDims)
{
    SceneRendererParams rv;
    rv.dimensions = renderDims;
    rv.antiAliasingLevel = antiAliasingLevel;
    rv.drawMeshNormals = false;
    rv.drawFloor = false;
    rv.viewMatrix = camera.view_matrix();
    rv.projectionMatrix = camera.projection_matrix(aspect_ratio(renderDims));
    rv.viewPos = camera.getPos();
    rv.lightDirection = RecommendedLightDirection(camera);
    rv.backgroundColor = {0.1f, 1.0f};
    return rv;
}

BVH osc::createTriangleBVHFromMesh(const Mesh& mesh)
{
    const auto indices = mesh.getIndices();

    BVH rv;
    if (indices.empty()) {
        return rv;
    }
    else if (mesh.getTopology() != MeshTopology::Triangles) {
        return rv;
    }
    else if (indices.isU32()) {
        rv.buildFromIndexedTriangles(mesh.getVerts() , indices.toU32Span());
    }
    else {
        rv.buildFromIndexedTriangles(mesh.getVerts(), indices.toU16Span());
    }
    return rv;
}

FrustumPlanes osc::calcCameraFrustumPlanes(const Camera& camera, float aspectRatio)
{
    const Radians fovY = camera.getVerticalFOV();
    const float zNear = camera.getNearClippingPlane();
    const float zFar = camera.getFarClippingPlane();
    const float halfVSize = zFar * tan(fovY * 0.5f);
    const float halfHSize = halfVSize * aspectRatio;
    const Vec3 pos = camera.getPosition();
    const Vec3 front = camera.getDirection();
    const Vec3 up = camera.getUpwardsDirection();
    const Vec3 right = cross(front, up);
    const Vec3 frontMultnear = zNear * front;
    const Vec3 frontMultfar = zFar * front;

    return {
        // origin            // normal
        to_analytic_plane(pos + frontMultnear, -front),                                                 // near
        to_analytic_plane(pos + frontMultfar ,  front),                                                 // far
        to_analytic_plane(pos                , -normalize(cross(frontMultfar - right*halfHSize, up))),  // right
        to_analytic_plane(pos                , -normalize(cross(up, frontMultfar + right*halfHSize))),  // left
        to_analytic_plane(pos                , -normalize(cross(right, frontMultfar - up*halfVSize))),  // top
        to_analytic_plane(pos                , -normalize(cross(frontMultfar + up*halfVSize, right))),  // bottom
    };
}
