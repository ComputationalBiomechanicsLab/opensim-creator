#include "PolarPerspectiveCamera.hpp"

#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/Rect.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>


namespace
{
    glm::vec3 PolarToCartesian(glm::vec3 focus, float radius, float theta, float phi)
    {
        float x = radius * std::sin(theta) * std::cos(phi);
        float y = radius * std::sin(phi);
        float z = radius * std::cos(theta) * std::cos(phi);

        return -focus + glm::vec3{x, y, z};
    }
}

osc::PolarPerspectiveCamera::PolarPerspectiveCamera() :
    radius{1.0f},
    theta{0.0f},
    phi{0.0f},
    focusPoint{0.0f, 0.0f, 0.0f},
    fov{120.0f},
    znear{0.1f},
    zfar{100.0f}
{
}

void osc::PolarPerspectiveCamera::reset()
{
    *this = {};
}

void osc::PolarPerspectiveCamera::pan(float aspectRatio, glm::vec2 delta) noexcept
{
    // how much panning is done depends on how far the camera is from the
    // origin (easy, with polar coordinates) *and* the FoV of the camera.
    float xAmt = delta.x * aspectRatio * (2.0f * std::tan(fov / 2.0f) * radius);
    float yAmt = -delta.y * (1.0f / aspectRatio) * (2.0f * std::tan(fov / 2.0f) * radius);

    // this assumes the scene is not rotated, so we need to rotate these
    // axes to match the scene's rotation
    glm::vec4 defaultPanningAx = {xAmt, yAmt, 0.0f, 1.0f};
    auto rotTheta = glm::rotate(glm::identity<glm::mat4>(), theta, glm::vec3{0.0f, 1.0f, 0.0f});
    auto thetaVec = glm::normalize(glm::vec3{std::sin(theta), 0.0f, std::cos(theta)});
    auto phiAxis = glm::cross(thetaVec, glm::vec3{0.0, 1.0f, 0.0f});
    auto rotPhi = glm::rotate(glm::identity<glm::mat4>(), phi, phiAxis);

    glm::vec4 panningAxes = rotPhi * rotTheta * defaultPanningAx;
    focusPoint.x += panningAxes.x;
    focusPoint.y += panningAxes.y;
    focusPoint.z += panningAxes.z;
}

void osc::PolarPerspectiveCamera::drag(glm::vec2 delta) noexcept
{
    theta += 2.0f * fpi * -delta.x;
    phi += 2.0f * fpi * delta.y;
}

void osc::PolarPerspectiveCamera::rescaleZNearAndZFarBasedOnRadius() noexcept
{
    // znear and zfar are only really dicated by the camera's radius, because
    // the radius is effectively the distance from the camera's focal point

    znear = 0.02f * radius;
    zfar = 20.0f * radius;
}

glm::mat4 osc::PolarPerspectiveCamera::getViewMtx() const noexcept
{
    // camera: at a fixed position pointing at a fixed origin. The "camera"
    // works by translating + rotating all objects around that origin. Rotation
    // is expressed as polar coordinates. Camera panning is represented as a
    // translation vector.

    // this maths is a complete shitshow and I apologize. It just happens to work for now. It's a polar coordinate
    // system that shifts the world based on the camera pan

    auto rotTheta = glm::rotate(glm::identity<glm::mat4>(), -theta, glm::vec3{0.0f, 1.0f, 0.0f});
    auto thetaVec = glm::normalize(glm::vec3{std::sin(theta), 0.0f, std::cos(theta)});
    auto phiAxis = glm::cross(thetaVec, glm::vec3{0.0, 1.0f, 0.0f});
    auto rotPhi = glm::rotate(glm::identity<glm::mat4>(), -phi, phiAxis);
    auto panTranslate = glm::translate(glm::identity<glm::mat4>(), focusPoint);
    return glm::lookAt(
        glm::vec3(0.0f, 0.0f, radius),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3{0.0f, 1.0f, 0.0f}) * rotTheta * rotPhi * panTranslate;
}

glm::mat4 osc::PolarPerspectiveCamera::getProjMtx(float aspectRatio) const noexcept
{
    return glm::perspective(fov, aspectRatio, znear, zfar);
}

glm::vec3 osc::PolarPerspectiveCamera::getPos() const noexcept
{
    return PolarToCartesian(focusPoint, radius, theta, phi);
}

glm::vec2 osc::PolarPerspectiveCamera::projectOntoScreenRect(glm::vec3 const& worldspaceLoc, Rect const& screenRect) const noexcept
{
    glm::vec2 dims = Dimensions(screenRect);
    glm::mat4 MV = getProjMtx(dims.x/dims.y) * getViewMtx();

    glm::vec4 ndc = MV * glm::vec4{worldspaceLoc, 1.0f};
    ndc /= ndc.w;  // perspective divide

    glm::vec2 ndc2D;
    ndc2D = {ndc.x, -ndc.y};        // [-1, 1], Y points down
    ndc2D += 1.0f;                  // [0, 2]
    ndc2D *= 0.5f;                  // [0, 1]
    ndc2D *= dims;                  // [0, w]
    ndc2D += screenRect.p1;         // [x, x + w]

    return ndc2D;
}

osc::Line osc::PolarPerspectiveCamera::unprojectTopLeftPosToWorldRay(glm::vec2 pos, glm::vec2 dims) const noexcept
{
    glm::mat4 projMtx = getProjMtx(dims.x/dims.y);
    glm::mat4 viewMtx = getViewMtx();

    // position of point, as if it were on the front of the 3D NDC cube
    glm::vec4 lineOriginNDC = TopleftRelPosToNDCCube(pos/dims);

    glm::vec4 lineOriginView = glm::inverse(projMtx) * lineOriginNDC;
    lineOriginView /= lineOriginView.w;  // perspective divide

    // location of mouse in worldspace
    glm::vec3 lineOriginWorld = glm::vec3{glm::inverse(viewMtx) * lineOriginView};

    // direction vector from camera to mouse location (i.e. the projection)
    glm::vec3 lineDirWorld = glm::normalize(lineOriginWorld - this->getPos());

    Line rv;
    rv.dir = lineDirWorld;
    rv.origin = lineOriginWorld;
    return rv;
}

osc::PolarPerspectiveCamera osc::CreateCameraWithRadius(float r)
{
    PolarPerspectiveCamera rv;
    rv.radius = r;
    return rv;
}

glm::vec3 osc::RecommendedLightDirection(osc::PolarPerspectiveCamera const& c)
{
    glm::vec3 p = PolarToCartesian(c.focusPoint, c.radius, c.theta - 0.75f*fpi4, glm::clamp(c.phi + (c.phi > 0.0f ? 0.25f*fpi4 : -0.25f*fpi4), -fpi2 + 0.05f, fpi2 - 0.05f));
    return glm::normalize(-c.focusPoint - p);
}

void osc::FocusAlongX(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = osc::fpi2;
    camera.phi = 0.0f;
}

void osc::FocusAlongMinusX(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = -osc::fpi2;
    camera.phi = 0.0f;
}

void osc::FocusAlongY(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = 0.0f;
    camera.phi = osc::fpi2;
}

void osc::FocusAlongMinusY(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = 0.0f;
    camera.phi = -osc::fpi2;
}

void osc::FocusAlongZ(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = 0.0f;
    camera.phi = 0.0f;
}

void osc::FocusAlongMinusZ(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = osc::fpi;
    camera.phi = 0.0f;
}

void osc::ZoomIn(osc::PolarPerspectiveCamera& camera)
{
    camera.radius *= 0.8f;
}

void osc::ZoomOut(osc::PolarPerspectiveCamera& camera)
{
    camera.radius *= 1.2f;
}

void osc::Reset(osc::PolarPerspectiveCamera& camera)
{
    camera = {};
    camera.theta = osc::fpi4;
    camera.phi = osc::fpi4;
}

void osc::AutoFocus(PolarPerspectiveCamera& camera, AABB const& elementAABB)
{
    camera.focusPoint = -Midpoint(elementAABB);
    camera.radius = 2.0f * LongestDim(elementAABB);
    camera.theta = osc::fpi4;
    camera.phi = osc::fpi4;
}
