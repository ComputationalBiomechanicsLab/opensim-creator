#pragma once

#include "src/Maths/Line.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace osc { struct AABB; }
namespace osc { struct Rect; }

namespace osc
{
    // a camera that focuses on and swivels around a focal point (e.g. for 3D model viewers)
    class PolarPerspectiveCamera final {
    public:

        PolarPerspectiveCamera();

        // reset the camera to its initial state
        void reset();

        // note: relative deltas here are relative to whatever "screen" the camera
        // is handling.
        //
        // e.g. moving a mouse 400px in X in a screen that is 800px wide should
        //      have a delta.x of 0.5f

        // pan: pan along the current view plane
        void pan(float aspectRatio, glm::vec2 mouseDelta) noexcept;

        // drag: spin the view around the origin, such that the distance between
        //       the camera and the origin remains constant
        void drag(glm::vec2 mouseDelta) noexcept;

        // autoscale znear and zfar based on the camera's distance from what it's looking at
        //
        // important for looking at extremely small/large scenes. znear and zfar dictates
        // both the culling planes of the camera *and* rescales the Z values of elements
        // in the scene. If the znear-to-zfar range is too large then Z-fighting will happen
        // and the scene will look wrong.
        void rescaleZNearAndZFarBasedOnRadius() noexcept;

        glm::mat4 getViewMtx() const noexcept;
        glm::mat4 getProjMtx(float aspect_ratio) const noexcept;

        // project's a worldspace coordinate onto a screen-space rectangle
        glm::vec2 projectOntoScreenRect(glm::vec3 const& worldspaceLoc, Rect const& screenRect) const noexcept;

        glm::vec3 getPos() const noexcept;

        // converts a `pos` (top-left) in the output `dims` into a line in worldspace by unprojection
        Line unprojectTopLeftPosToWorldRay(glm::vec2 pos, glm::vec2 dims) const noexcept;

        float radius;
        float theta;
        float phi;
        glm::vec3 focusPoint;
        float fov;
        float znear;
        float zfar;
    };

    bool operator==(PolarPerspectiveCamera const&, PolarPerspectiveCamera const&) noexcept;
    bool operator!=(PolarPerspectiveCamera const&, PolarPerspectiveCamera const&) noexcept;

    PolarPerspectiveCamera CreateCameraWithRadius(float);
    PolarPerspectiveCamera CreateCameraFocusedOn(AABB const&);
    glm::vec3 RecommendedLightDirection(PolarPerspectiveCamera const&);
    void FocusAlongX(PolarPerspectiveCamera&);
    void FocusAlongMinusX(PolarPerspectiveCamera&);
    void FocusAlongY(PolarPerspectiveCamera&);
    void FocusAlongMinusY(PolarPerspectiveCamera&);
    void FocusAlongZ(PolarPerspectiveCamera&);
    void FocusAlongMinusZ(PolarPerspectiveCamera&);
    void ZoomIn(PolarPerspectiveCamera&);
    void ZoomOut(PolarPerspectiveCamera&);
    void Reset(PolarPerspectiveCamera&);
    void AutoFocus(PolarPerspectiveCamera&, AABB const& elementAABB, float aspectRatio = 1.0f);
}
