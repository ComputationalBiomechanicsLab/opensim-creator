#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

namespace osc { struct AABB; }
namespace osc { struct Rect; }

namespace osc
{
    // a camera that focuses on and swivels around a focal point (e.g. for 3D model viewers)
    struct PolarPerspectiveCamera final {

        PolarPerspectiveCamera();

        // reset the camera to its initial state
        void reset();

        // note: relative deltas here are relative to whatever "screen" the camera
        // is handling.
        //
        // e.g. moving a mouse 400px in X in a screen that is 800px wide should
        //      have a delta.x of 0.5f

        // pan: pan along the current view plane
        void pan(float aspectRatio, Vec2 mouseDelta);

        // drag: spin the view around the origin, such that the distance between
        //       the camera and the origin remains constant
        void drag(Vec2 mouseDelta);

        // autoscale znear and zfar based on the camera's distance from what it's looking at
        //
        // important for looking at extremely small/large scenes. znear and zfar dictates
        // both the culling planes of the camera *and* rescales the Z values of elements
        // in the scene. If the znear-to-zfar range is too large then Z-fighting will happen
        // and the scene will look wrong.
        void rescaleZNearAndZFarBasedOnRadius();

        Mat4 view_matrix() const;
        Mat4 projection_matrix(float aspectRatio) const;

        // project's a worldspace coordinate onto a screen-space rectangle
        Vec2 projectOntoScreenRect(Vec3 const& worldspaceLoc, Rect const& screenRect) const;

        Vec3 getPos() const;

        // converts a `pos` (top-left) in the output `dims` into a line in worldspace by unprojection
        Line unprojectTopLeftPosToWorldRay(Vec2 pos, Vec2 dims) const;

        friend bool operator==(PolarPerspectiveCamera const&, PolarPerspectiveCamera const&) = default;

        float radius;
        Radians theta;
        Radians phi;
        Vec3 focusPoint;
        Radians vertical_fov;
        float znear;
        float zfar;
    };

    PolarPerspectiveCamera CreateCameraWithRadius(float);
    PolarPerspectiveCamera CreateCameraFocusedOn(AABB const&);
    Vec3 recommended_light_direction(PolarPerspectiveCamera const&);
    void FocusAlongAxis(PolarPerspectiveCamera&, size_t, bool negate = false);
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
