// This file was originally copied from a third-party source:
//
// - https://github.com/CedricGuillemet/ImGuizmo
// - commit: 6d588209f99b1324a608783d1f52faa518886c29
// - Licensed under the MIT license
// - Copyright(c) 2021 Cedric Guillemet
//
// Subsequent modifications to this file (see VCS) are copyright (c) 2024 Adam Kewley.
//
// ORIGINAL LICENSE TEXT:
//
// https://github.com/CedricGuillemet/ImGuizmo
// v1.91.3 WIP
//
// The MIT License(MIT)
//
// Copyright(c) 2021 Cedric Guillemet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// NOLINTBEGIN

#ifndef IMGUI_DEFINE_MATH_OPERATORS
    #define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "imgui_internal.h"
#include "imguizmo.h"

#include <liboscar/maths/constants.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/matrix_functions.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/transform_functions.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/scope_exit.h>

#include <array>
#include <functional>
#include <limits>
#include <optional>
#include <vector>

using namespace osc;
using namespace osc::ui::gizmo::detail;

namespace
{
    constexpr float c_screen_rotate_size = 0.06f;

    // scale a bit so translate axis do not touch when in universal
    constexpr float c_rotation_display_factor = 1.2f;

    constexpr auto c_translation_info_mask = std::to_array<CStringView>({
        "X : %5.3f",
        "Y : %5.3f",
        "Z : %5.3f",
        "Y : %5.3f Z : %5.3f",
        "X : %5.3f Z : %5.3f",
        "X : %5.3f Y : %5.3f",
        "X : %5.3f Y : %5.3f Z : %5.3f",
    });

    constexpr auto c_scale_info_mask = std::to_array<CStringView>({
        "X : %5.2f",
        "Y : %5.2f",
        "Z : %5.2f",
        "XYZ : %5.2f",
    });

    constexpr auto c_rotation_info_mask = std::to_array<CStringView>({
        "X : %5.2f deg %5.2f rad",
        "Y : %5.2f deg %5.2f rad",
        "Z : %5.2f deg %5.2f rad",
        "Screen : %5.2f deg %5.2f rad",
    });

    constexpr auto c_translation_info_index = std::to_array<int>({
        0,0,0,
        1,0,0,
        2,0,0,
        1,2,0,
        0,2,0,
        0,1,0,
        0,1,2,
    });

    constexpr float c_quad_min = 0.5f;
    constexpr float c_quad_max = 0.8f;
    constexpr auto c_quad_uv = std::to_array<float>({
        c_quad_min, c_quad_min,
        c_quad_min, c_quad_max,
        c_quad_max, c_quad_max,
        c_quad_max, c_quad_min,
    });
    constexpr int c_half_circle_segment_count = 64;
    constexpr float c_snap_tension = 0.5f;

    enum COLOR {
        DIRECTION_X,      // directionColor[0]
        DIRECTION_Y,      // directionColor[1]
        DIRECTION_Z,      // directionColor[2]
        PLANE_X,          // planeColor[0]
        PLANE_Y,          // planeColor[1]
        PLANE_Z,          // planeColor[2]
        SELECTION,        // selectionColor
        INACTIVE,         // inactiveColor
        TRANSLATION_LINE, // translationLineColor
        SCALE_LINE,
        ROTATION_USING_BORDER,
        ROTATION_USING_FILL,
        HATCHED_AXIS_LINES,
        TEXT,
        TEXT_SHADOW,
        COUNT
    };

    constexpr auto c_direction_unary = std::to_array<Vector4>({
        {1.f, 0.f, 0.f, 0.0f},
        {0.f, 1.f, 0.f, 0.0f},
        {0.f, 0.f, 1.f, 0.0f},
    });

    enum MOVETYPE {
        MT_NONE,
        MT_MOVE_X,
        MT_MOVE_Y,
        MT_MOVE_Z,
        MT_MOVE_YZ,
        MT_MOVE_ZX,
        MT_MOVE_XY,
        MT_MOVE_SCREEN,
        MT_ROTATE_X,
        MT_ROTATE_Y,
        MT_ROTATE_Z,
        MT_ROTATE_SCREEN,
        MT_SCALE_X,
        MT_SCALE_Y,
        MT_SCALE_Z,
        MT_SCALE_XYZ
    };

    // Matches MT_MOVE_AB order
    constexpr auto TRANSLATE_PLANS = std::to_array<Operation>({
        Operation::TranslateY | Operation::TranslateZ,
        Operation::TranslateX | Operation::TranslateZ,
        Operation::TranslateX | Operation::TranslateY,
    });

    struct Style {
        Style()
        {
            Colors[DIRECTION_X]           = ImVec4(0.666f, 0.000f, 0.000f, 1.000f);
            Colors[DIRECTION_Y]           = ImVec4(0.000f, 0.666f, 0.000f, 1.000f);
            Colors[DIRECTION_Z]           = ImVec4(0.000f, 0.000f, 0.666f, 1.000f);
            Colors[PLANE_X]               = ImVec4(0.666f, 0.000f, 0.000f, 0.380f);
            Colors[PLANE_Y]               = ImVec4(0.000f, 0.666f, 0.000f, 0.380f);
            Colors[PLANE_Z]               = ImVec4(0.000f, 0.000f, 0.666f, 0.380f);
            Colors[SELECTION]             = ImVec4(1.000f, 0.500f, 0.062f, 0.541f);
            Colors[INACTIVE]              = ImVec4(0.600f, 0.600f, 0.600f, 0.600f);
            Colors[TRANSLATION_LINE]      = ImVec4(0.666f, 0.666f, 0.666f, 0.666f);
            Colors[SCALE_LINE]            = ImVec4(0.250f, 0.250f, 0.250f, 1.000f);
            Colors[ROTATION_USING_BORDER] = ImVec4(1.000f, 0.500f, 0.062f, 1.000f);
            Colors[ROTATION_USING_FILL]   = ImVec4(1.000f, 0.500f, 0.062f, 0.500f);
            Colors[HATCHED_AXIS_LINES]    = ImVec4(0.000f, 0.000f, 0.000f, 0.500f);
            Colors[TEXT]                  = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
            Colors[TEXT_SHADOW]           = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
        }

        float TranslationLineThickness   = 5.0f;  // Thickness of lines for translation gizmo
        float TranslationLineArrowSize   = 8.0f;  // Size of arrow at the end of lines for translation gizmo
        float RotationLineThickness      = 5.0f;  // Thickness of lines for rotation gizmo
        float RotationOuterLineThickness = 7.0f;  // Thickness of line surrounding the rotation gizmo
        float ScaleLineThickness         = 5.0f;  // Thickness of lines for scale gizmo
        float ScaleLineCircleSize        = 8.0f;  // Size of circle at the end of lines for scale gizmo
        float HatchedAxisLineThickness   = 6.0f;  // Thickness of hatched axis lines
        float CenterCircleSize           = 6.0f;  // Size of circle at the center of the translate/scale gizmo

        ImVec4 Colors[COLOR::COUNT];
    };

    constexpr ImGuiID blank_id()
    {
        return std::numeric_limits<ImGuiID>::max();
    }

    // True if lhs contains rhs
    bool Contains(Operation lhs, Operation rhs)
    {
        return static_cast<Operation>(std::to_underlying(lhs) & std::to_underlying(rhs)) == rhs;
    }

    template<typename T>
    bool IsWithin(T x, T y, T z)
    {
        return (x >= y) and (x <= z);
    }

    Vector4 BuildPlan(const Vector3& p_point1, const Vector3& p_normal)
    {
        const Vector3 normal = normalize(p_normal);
        return {normal, dot(normal, p_point1)};
    }

    Vector4 imguizmo_transform_vector(const Matrix4x4& m, const Vector4& v)
    {
        return m * Vector4{Vector3{v}, 0.0f};
    }

    Vector4 imguizmo_transform_point(const Matrix4x4& m, const Vector4& p)
    {
        return m * Vector4{Vector3{p}, 1.0f};
    }

    Vector4 imguizmo_cross(const Vector4& lhs, const Vector4& rhs)
    {
        // This is a shim for ImGuizmo's legacy behavior
        const Vector3 cp = cross(Vector3{lhs}, Vector3{rhs});
        return {cp, 0.0f};
    }

    bool IsTranslateType(int type)
    {
        return type >= MT_MOVE_X && type <= MT_MOVE_SCREEN;
    }

    bool IsRotateType(int type)
    {
        return type >= MT_ROTATE_X && type <= MT_ROTATE_SCREEN;
    }

    bool IsScaleType(int type)
    {
        return type >= MT_SCALE_X && type <= MT_SCALE_XYZ;
    }

    float GetParallelogram(
        const Matrix4x4& MVP,
        float displayRatio,
        const Vector4& ptO,
        const Vector4& ptA,
        const Vector4& ptB)
    {
        auto pts = std::to_array({ptO, ptA, ptB});
        for (auto& pt: pts) {
            pt = imguizmo_transform_point(MVP, pt);
            if (fabsf(pt.w()) > FLT_EPSILON) {
                // check for axis aligned with camera direction
                pt *= 1.f / pt.w();
            }
        }
        Vector4 segA = pts[1] - pts[0];
        Vector4 segB = pts[2] - pts[0];
        segA.y() /= displayRatio;
        segB.y() /= displayRatio;
        const Vector4 segAOrtho = {normalize(Vector2{-segA.y(), segA.x()}), 0.0f, 0.0f};
        float dt = dot<float, 3>(segAOrtho, segB);
        float surface = length(segA.xy()) * fabsf(dt);
        return surface;
    }

    float GetSegmentLengthClipSpace(
        const Matrix4x4& mvp,
        float displayRatio,
        const Vector4& start,
        const Vector4& end)
    {
        Vector4 startOfSegment = imguizmo_transform_point(mvp, start);
        if (fabsf(startOfSegment.w()) > FLT_EPSILON) {
            // check for axis aligned with camera direction
            startOfSegment *= 1.f / startOfSegment.w();
        }

        Vector4 endOfSegment = imguizmo_transform_point(mvp, end);
        if (fabsf(endOfSegment.w()) > FLT_EPSILON) {
            // check for axis aligned with camera direction
            endOfSegment *= 1.f / endOfSegment.w();
        }

        Vector4 clipSpaceAxis = endOfSegment - startOfSegment;
        if (displayRatio < 1.0) {
            clipSpaceAxis.x() *= displayRatio;
        } else {
            clipSpaceAxis.y() /= displayRatio;
        }

        return length(clipSpaceAxis.xy());
    }

    float IntersectRayPlane(
        const Vector4& rOrigin,
        const Vector4& rVector,
        const Vector4& plan)
    {
        const float numer = dot<float, 3>(plan, rOrigin) - plan.w();
        const float denom = dot<float, 3>(plan, rVector);

        if (fabsf(denom) < FLT_EPSILON) {
            // normal is orthogonal to vector, cant intersect
            return -1.0f;
        }

        return -(numer / denom);
    }

    ImVec2 worldToPos(
        const Vector4& worldPos,
        const Matrix4x4& mat,
        ImVec2 position,
        ImVec2 size)
    {
        Vector4 trans = imguizmo_transform_point(mat, worldPos);
        trans *= 0.5f / trans.w();
        trans += Vector4{0.5f, 0.5f, 0.0f, 0.0f};
        trans.y() = 1.f - trans.y();
        trans.x() *= size.x;
        trans.y() *= size.y;
        trans.x() += position.x;
        trans.y() += position.y;
        return ImVec2(trans.x(), trans.y());
    }

    void ComputeCameraRay(
        const Matrix4x4& viewMat,
        const Matrix4x4& projectionMat,
        bool reversed,
        Vector4& rayOrigin,
        Vector4& rayDir,
        ImVec2 position,
        ImVec2 size)
    {
        ImGuiIO& io = ImGui::GetIO();

        Matrix4x4 mViewProjInverse = inverse(projectionMat * viewMat);

        const float mox = ((io.MousePos.x - position.x) / size.x) * 2.f - 1.f;
        const float moy = (1.f - ((io.MousePos.y - position.y) / size.y)) * 2.f - 1.f;

        const float zNear = reversed ? (1.f - FLT_EPSILON) : 0.f;
        const float zFar = reversed ? 0.f : (1.f - FLT_EPSILON);

        rayOrigin = mViewProjInverse * Vector4{mox, moy, zNear, 1.f};
        rayOrigin *= 1.f / rayOrigin.w();
        Vector4 rayEnd = mViewProjInverse * Vector4{mox, moy, zFar, 1.0f};
        rayEnd *= 1.f / rayEnd.w();
        rayDir = normalize(rayEnd - rayOrigin);
    }

    void OrthoNormalize(Matrix4x4& m)
    {
        m[0] = normalize(m[0]);
        m[1] = normalize(m[1]);
        m[2] = normalize(m[2]);
        // ignore 3rd column (position)
    }

    Vector4 PointOnSegment(
        const Vector4& point,
        const Vector4& vertPos1,
        const Vector4& vertPos2)
    {
        Vector4 c = point - vertPos1;
        Vector4 V = normalize(vertPos2 - vertPos1);
        float d = length(vertPos2 - vertPos1);
        float t = dot<float, 3>(V, c);

        if (t < 0.f) {
            return vertPos1;
        }

        if (t > d) {
            return vertPos2;
        }

        return vertPos1 + V * t;
    }

    bool CanActivate()
    {
        return ImGui::IsMouseClicked(0) and not ImGui::IsAnyItemHovered() and not ImGui::IsAnyItemActive();
    }

    void ComputeSnap(float* value, float snap)
    {
        if (snap <= FLT_EPSILON) {
            return;
        }

        float modulo = fmodf(*value, snap);
        float moduloRatio = fabsf(modulo) / snap;
        if (moduloRatio < c_snap_tension) {
            *value -= modulo;
        } else if (moduloRatio > (1.f - c_snap_tension)) {
            *value = *value - modulo + snap * ((*value < 0.f) ? -1.f : 1.f);
        }
    }

    void ComputeSnap(Vector4& value, const Vector3& snap)
    {
        for (int i = 0; i < 3; i++) {
            ComputeSnap(&value[i], snap[i]);
        }
    }
}

struct osc::ui::gizmo::detail::GizmoContext::Impl {
    Impl()
    {
        mIDStack.push_back(blank_id());
    }

    void SetDrawlist(ImDrawList* drawlist)
    {
        mDrawList = drawlist ? drawlist : ImGui::GetWindowDrawList();
    }

    void BeginFrame()
    {
        const ImU32 flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
        ImGui::PushStyleColor(ImGuiCol_Border, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        ImGui::Begin("gizmo", NULL, flags);
        mDrawList = ImGui::GetWindowDrawList();
        mbOverGizmoHotspot = false;
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    bool IsOver()
    {
        return IsOver(mOperation);
    }

    bool IsOver(Operation op)
    {
        return
            IsUsing()
            or ((op & Operation::Scale) and GetScaleType(op) != MT_NONE)
            or ((op & Operation::Rotate) and GetRotateType(op) != MT_NONE)
            or ((op & Operation::Translate) and GetMoveType(op, NULL) != MT_NONE);
    }

    bool IsUsing() const
    {
        return (mbUsing and (GetCurrentID() == mEditingID)) or mbUsingBounds;
    }

    bool IsUsingAny() const
    {
        return mbUsing or mbUsingBounds;
    }

    void Enable(bool enable)
    {
        mbEnable = enable;
        if (not enable) {
            mbUsing = false;
            mbUsingBounds = false;
        }
    }

    void SetRect(const Rect& ui_rect)
    {
        mX = ui_rect.left();
        mY = ui_rect.ypd_top();
        mWidth = ui_rect.width();
        mHeight = ui_rect.height();
        mXMax = mX + mWidth;
        mYMax = mY + mXMax;
        mDisplayRatio = aspect_ratio_of(ui_rect);
    }

    void SetOrthographic(bool isOrthographic)
    {
        mIsOrthographic = isOrthographic;
    }

    ImGuiID GetID(int n) const
    {
        ImGuiID seed = mIDStack.back();
        ImGuiID id = ImHashData(&n, sizeof(n), seed);
        return id;
    }

    ImGuiID GetCurrentID() const
    {
        return mIDStack.back();
    }

    void PushID(UID uid)
    {
        ImGuiID id = GetID(static_cast<int>(std::hash<UID>{}(uid)));
        mIDStack.push_back(id);
    }

    void PopID()
    {
        IM_ASSERT(context.mIDStack.Size > 1); // Too many PopID(), or could be popping in a wrong/different window?
        mIDStack.pop_back();
    }

    void SetGizmoSizeClipSpace(float value)
    {
        mGizmoSizeClipSpace = value;
    }

    void SetAxisLimit(float value)
    {
        mAxisLimit = value;
    }

    void SetAxisMask(bool x, bool y, bool z)
    {
        mAxisMask = (x ? 1 : 0) + (y ? 2 : 0) + (z ? 4 : 0);
    }

    void SetPlaneLimit(float value)
    {
        mPlaneLimit = value;
    }

    int GetScaleType(Operation op)
    {
        if (mbUsing) {
            return MT_NONE;
        }

        ImGuiIO& io = ImGui::GetIO();
        int type = MT_NONE;

        // screen
        if (io.MousePos.x >= mScreenSquareMin.x && io.MousePos.x <= mScreenSquareMax.x &&
            io.MousePos.y >= mScreenSquareMin.y && io.MousePos.y <= mScreenSquareMax.y &&
            Contains(op, Operation::Scale)) {
            type = MT_SCALE_XYZ;
        }

        // compute
        for (int i = 0; i < 3 && type == MT_NONE; i++) {
            if (not(op & (Operation::ScaleX << i))) {
                continue;
            }
            bool isAxisMasked = (1 << i) & mAxisMask;

            Vector4 dirPlaneX, dirPlaneY, dirAxis;
            bool belowAxisLimit, belowPlaneLimit;
            ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);
            dirAxis = imguizmo_transform_vector(mModelLocal, dirAxis);
            dirPlaneX = imguizmo_transform_vector(mModelLocal, dirPlaneX);
            dirPlaneY = imguizmo_transform_vector(mModelLocal, dirPlaneY);

            const float len = IntersectRayPlane(mRayOrigin, mRayVector, BuildPlan(mModelLocal[3], dirAxis));
            Vector4 posOnPlan = mRayOrigin + mRayVector * len;

            const float startOffset = Contains(op, Operation::TranslateX << i) ? 1.0f : 0.1f;
            const float endOffset = Contains(op, Operation::TranslateX << i) ? 1.4f : 1.0f;
            const ImVec2 posOnPlanScreen = worldToPos(posOnPlan, mViewProjection);
            const ImVec2 axisStartOnScreen = worldToPos(mModelLocal[3] + dirAxis * mScreenFactor * startOffset,
                                                        mViewProjection);
            const ImVec2 axisEndOnScreen = worldToPos(mModelLocal[3] + dirAxis * mScreenFactor * endOffset,
                                                      mViewProjection);

            Vector4 closestPointOnAxis = PointOnSegment(
                Vector4{posOnPlanScreen.x, posOnPlanScreen.y, 0.0f, 0.0f},
                Vector4{axisStartOnScreen.x, axisStartOnScreen.y, 0.0f, 0.0f},
                Vector4{axisEndOnScreen.x, axisEndOnScreen.y, 0.0f, 0.0f}
            );

            if (length(closestPointOnAxis - Vector4{posOnPlanScreen.x, posOnPlanScreen.y, 0.0f, 0.0f}) < 12.f) {  // pixel size
                if (not isAxisMasked) {
                    type = MT_SCALE_X + i;
                }
            }
        }

        // universal

        Vector4 deltaScreen = {io.MousePos.x - mScreenSquareCenter.x, io.MousePos.y - mScreenSquareCenter.y, 0.f, 0.f};
        float dist = length(deltaScreen);
        if (Contains(op, Operation::ScaleU) && dist >= 17.0f && dist < 23.0f) {
            type = MT_SCALE_XYZ;
        }

        for (int i = 0; i < 3 && type == MT_NONE; i++) {

            if (not(op & (Operation::ScaleXU << i))) {
                continue;
            }

            Vector4 dirPlaneX, dirPlaneY, dirAxis;
            bool belowAxisLimit, belowPlaneLimit;
            ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);

            // draw axis
            if (belowAxisLimit) {
                bool hasTranslateOnAxis = Contains(op, Operation::TranslateX << i);
                float markerScale = hasTranslateOnAxis ? 1.4f : 1.0f;
                //ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * context.mScreenFactor, context.mMVPLocal);
                //ImVec2 worldDirSSpaceNoScale = worldToPos(dirAxis * markerScale * context.mScreenFactor, context.mMVP);
                ImVec2 worldDirSSpace = worldToPos((dirAxis * markerScale) * mScreenFactor, mMVPLocal);

                float distance = sqrtf(ImLengthSqr(worldDirSSpace - io.MousePos));
                if (distance < 12.f) {
                    type = MT_SCALE_X + i;
                }
            }
        }
        return type;
    }

    int GetRotateType(Operation op)
    {
        if (mbUsing) {
            return MT_NONE;
        }

        bool isNoAxesMasked = not mAxisMask;
        bool isMultipleAxesMasked = mAxisMask & (mAxisMask - 1);

        ImGuiIO& io = ImGui::GetIO();
        int type = MT_NONE;

        Vector4 deltaScreen = {io.MousePos.x - mScreenSquareCenter.x, io.MousePos.y - mScreenSquareCenter.y, 0.f, 0.f};
        float dist = length(deltaScreen);
        if ((op & Operation::RotateInScreen) && dist >= (mRadiusSquareCenter - 4.0f) && dist < (mRadiusSquareCenter + 4.0f)) {
            if (not isNoAxesMasked) {
                return MT_NONE;
            }
            type = MT_ROTATE_SCREEN;
        }

        const Vector4 planNormals[] = {mModel[0], mModel[1], mModel[2]};

        Vector4 modelViewPos = imguizmo_transform_point(mViewMat, mModel[3]);

        for (int i = 0; i < 3 && type == MT_NONE; i++) {
            if (not(op & (Operation::RotateX << i))) {
                continue;
            }
            bool isAxisMasked = (1 << i) & mAxisMask;
            // pickup plan
            Vector4 pickupPlan = BuildPlan(mModel[3], planNormals[i]);

            const float len = IntersectRayPlane(mRayOrigin, mRayVector, pickupPlan);
            const Vector4 intersectWorldPos = mRayOrigin + mRayVector * len;
            Vector4 intersectViewPos = imguizmo_transform_point(mViewMat, intersectWorldPos);

            if (ImAbs(modelViewPos.z()) - ImAbs(intersectViewPos.z()) < -FLT_EPSILON) {
                continue;
            }

            const Vector4 localPos = intersectWorldPos - mModel[3];
            Vector4 idealPosOnCircle = imguizmo_transform_vector(mModelInverse, normalize(localPos));
            const ImVec2 idealPosOnCircleScreen = worldToPos(
                idealPosOnCircle * c_rotation_display_factor * mScreenFactor, mMVP);

            //context.mDrawList->AddCircle(idealPosOnCircleScreen, 5.f, IM_COL32_WHITE);
            const ImVec2 distanceOnScreen = idealPosOnCircleScreen - io.MousePos;

            const float distance = length(Vector4{distanceOnScreen.x, distanceOnScreen.y, 0.0f, 0.0f});
            if (distance < 8.f) {  // pixel size
                if ((not isAxisMasked or isMultipleAxesMasked) and not isNoAxesMasked) {
                    break;
                }
                type = MT_ROTATE_X + i;
            }
        }

        return type;
    }

    int GetMoveType(Operation op, Vector4* gizmoHitProportion)
    {
        if (not(op & Operation::Translate) or mbUsing or not mbMouseOver) {
            return MT_NONE;
        }

        bool isNoAxesMasked = not mAxisMask;
        bool isMultipleAxesMasked = mAxisMask & (mAxisMask - 1);

        ImGuiIO& io = ImGui::GetIO();
        int type = MT_NONE;

        // screen
        if (io.MousePos.x >= mScreenSquareMin.x && io.MousePos.x <= mScreenSquareMax.x &&
            io.MousePos.y >= mScreenSquareMin.y && io.MousePos.y <= mScreenSquareMax.y &&
            Contains(op, Operation::Translate)) {

            type = MT_MOVE_SCREEN;
        }

        const ImVec2 tmp = io.MousePos - ImVec2(mX, mY);
        const Vector4 screenCoord{tmp.x, tmp.y, 0.0f, 0.0f};

        // compute
        for (int i = 0; i < 3 && type == MT_NONE; i++) {

            bool isAxisMasked = (1 << i) & mAxisMask;
            Vector4 dirPlaneX, dirPlaneY, dirAxis;
            bool belowAxisLimit, belowPlaneLimit;
            ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
            dirAxis = imguizmo_transform_vector(mModel, dirAxis);
            dirPlaneX = imguizmo_transform_vector(mModel, dirPlaneX);
            dirPlaneY = imguizmo_transform_vector(mModel, dirPlaneY);

            const float len = IntersectRayPlane(mRayOrigin, mRayVector, BuildPlan(mModel[3], dirAxis));
            Vector4 posOnPlan = mRayOrigin + mRayVector * len;

            const ImVec2 axisStartOnScreen = worldToPos(mModel[3] + dirAxis * mScreenFactor * 0.1f, mViewProjection) - ImVec2(mX, mY);
            const ImVec2 axisEndOnScreen = worldToPos(mModel[3] + dirAxis * mScreenFactor, mViewProjection) - ImVec2(mX, mY);

            Vector4 closestPointOnAxis = PointOnSegment(
                screenCoord,
                Vector4{axisStartOnScreen.x, axisStartOnScreen.y, 0.0f, 0.0f},
                Vector4{axisEndOnScreen.x, axisEndOnScreen.y, 0.0f, 0.0f}
            );
            if (length(closestPointOnAxis - screenCoord) < 12.f and (op & (Operation::TranslateX << i))) // pixel size
            {
                if (isAxisMasked)
                    break;
                type = MT_MOVE_X + i;
            }

            const float dx = dot<float, 3>(dirPlaneX, (posOnPlan - mModel[3]) * (1.f / mScreenFactor));
            const float dy = dot<float, 3>(dirPlaneY, (posOnPlan - mModel[3]) * (1.f / mScreenFactor));
            if (belowPlaneLimit && dx >= c_quad_uv[0] && dx <= c_quad_uv[4] && dy >= c_quad_uv[1] && dy <= c_quad_uv[3]
                && Contains(op, TRANSLATE_PLANS[i])) {
                if ((!isAxisMasked || isMultipleAxesMasked) && !isNoAxesMasked)
                    break;
                type = MT_MOVE_YZ + i;
            }

            if (gizmoHitProportion) {
                *gizmoHitProportion = Vector4{dx, dy, 0.0f, 0.0f};
            }
        }
        return type;
    }

    std::optional<Transform> Manipulate(
        const Matrix4x4& view,
        const Matrix4x4& projection,
        Operation operation,
        Mode mode,
        Matrix4x4& matrix,
        const GizmoOperationSnappingSteps* snap,
        const AABB* localBounds,
        const float* boundsSnap)
    {
        mDrawList->PushClipRect(
            ImVec2(mX, mY),
            ImVec2(mX + mWidth, mY + mHeight),
            false
        );
        const ScopeExit popClipRectOnExit{[&] { mDrawList->PopClipRect(); }};

        // Scale is always local or matrix will be skewed when applying world scale or oriented matrix
        ComputeContext(view, projection, matrix, (operation & Operation::Scale) ? Mode::Local : mode);

        // behind camera
        Vector4 camSpacePosition = imguizmo_transform_point(mMVP, Vector4{});
        if (not mIsOrthographic and camSpacePosition.z() < 0.0001f and not mbUsing) {
            return std::nullopt;
        }

        // --
        int type = MT_NONE;
        bool manipulated = false;
        Matrix4x4 deltaMatrix;
        if (mbEnable) {
            if (not mbUsingBounds) {
                manipulated =
                        HandleTranslation(matrix, &deltaMatrix, operation, type, snap)
                        or HandleScale(matrix, &deltaMatrix, operation, type, snap)
                        or HandleRotation(matrix, &deltaMatrix, operation, type, snap);
            }
        }

        if (localBounds and not mbUsing) {
            static_assert(sizeof(AABB) == 6 * sizeof(float) and alignof(AABB) >= alignof(float));
            HandleAndDrawLocalBounds(reinterpret_cast<const float*>(&localBounds), &matrix, boundsSnap, operation);
        }

        mOperation = operation;
        if (not mbUsingBounds) {
            DrawRotationGizmo(operation, type);
            DrawTranslationGizmo(operation, type);
            DrawScaleGizmo(operation, type);
            DrawScaleUniveralGizmo(operation, type);
        }

        if (manipulated) {
            Transform rv;
            if (try_decompose_to_transform(deltaMatrix, rv)) {
                return rv;
            } else {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }
    }

    void ComputeTripodAxisAndVisibility(
        const int axisIndex,
        Vector4& dirAxis,
        Vector4& dirPlaneX,
        Vector4& dirPlaneY,
        bool& belowAxisLimit,
        bool& belowPlaneLimit,
        const bool localCoordinates = false)
    {
        dirAxis = c_direction_unary[axisIndex];
        dirPlaneX = c_direction_unary[(axisIndex + 1) % 3];
        dirPlaneY = c_direction_unary[(axisIndex + 2) % 3];

        if (mbUsing and (GetCurrentID() == mEditingID)) {
            // when using, use stored factors so the gizmo doesn't flip when we translate

            // Apply axis mask to axes and planes
            belowAxisLimit = mBelowAxisLimit[axisIndex] && ((1 << axisIndex) & mAxisMask);
            belowPlaneLimit = mBelowPlaneLimit[axisIndex] && ((((1 << axisIndex) & mAxisMask) && !(mAxisMask & (mAxisMask - 1))) || !mAxisMask);

            dirAxis *= mAxisFactor[axisIndex];
            dirPlaneX *= mAxisFactor[(axisIndex + 1) % 3];
            dirPlaneY *= mAxisFactor[(axisIndex + 2) % 3];
        } else {
            const Matrix4x4& activeMVP = localCoordinates ? mMVPLocal : mMVP;

            // new method
            float lenDir = GetSegmentLengthClipSpace(activeMVP, mDisplayRatio, Vector4{}, dirAxis);
            float lenDirMinus = GetSegmentLengthClipSpace(activeMVP, mDisplayRatio, Vector4{}, -dirAxis);

            float lenDirPlaneX = GetSegmentLengthClipSpace(activeMVP, mDisplayRatio, Vector4{}, dirPlaneX);
            float lenDirMinusPlaneX = GetSegmentLengthClipSpace(activeMVP, mDisplayRatio, Vector4{}, -dirPlaneX);

            float lenDirPlaneY = GetSegmentLengthClipSpace(activeMVP, mDisplayRatio, Vector4{}, dirPlaneY);
            float lenDirMinusPlaneY = GetSegmentLengthClipSpace(activeMVP, mDisplayRatio, Vector4{}, -dirPlaneY);

            // For readability
            bool allowFlip = mAllowAxisFlip;
            float mulAxis  = (allowFlip && lenDir       < lenDirMinus       && fabsf(lenDir       - lenDirMinus)       > FLT_EPSILON) ? -1.f : 1.f;
            float mulAxisX = (allowFlip && lenDirPlaneX < lenDirMinusPlaneX && fabsf(lenDirPlaneX - lenDirMinusPlaneX) > FLT_EPSILON) ? -1.f : 1.f;
            float mulAxisY = (allowFlip && lenDirPlaneY < lenDirMinusPlaneY && fabsf(lenDirPlaneY - lenDirMinusPlaneY) > FLT_EPSILON) ? -1.f : 1.f;
            dirAxis *= mulAxis;
            dirPlaneX *= mulAxisX;
            dirPlaneY *= mulAxisY;

            // for axis
            float axisLengthInClipSpace = GetSegmentLengthClipSpace(
                activeMVP,
                mDisplayRatio,
                Vector4{},
                dirAxis * mScreenFactor
            );

            float paraSurf = GetParallelogram(mMVP, mDisplayRatio, Vector4{}, dirPlaneX * mScreenFactor, dirPlaneY * mScreenFactor);

            // Apply axis mask to axes and planes
            belowPlaneLimit = (paraSurf > mAxisLimit) && ((((1 << axisIndex) & mAxisMask) && !(mAxisMask & (mAxisMask - 1))) || !mAxisMask);
            belowAxisLimit = (axisLengthInClipSpace > mPlaneLimit) && !((1 << axisIndex) & mAxisMask);

            // and store values
            mAxisFactor[axisIndex] = mulAxis;
            mAxisFactor[(axisIndex + 1) % 3] = mulAxisX;
            mAxisFactor[(axisIndex + 2) % 3] = mulAxisY;
            mBelowAxisLimit[axisIndex] = belowAxisLimit;
            mBelowPlaneLimit[axisIndex] = belowPlaneLimit;
        }
    }

    void ComputeContext(
        const Matrix4x4& view,
        const Matrix4x4& projection,
        Matrix4x4& matrix,
        Mode mode)
    {
        mMode = mode;
        mViewMat = view;
        mProjectionMat = projection;
        mbMouseOver = IsHoveringWindow();

        mModelLocal = matrix;
        OrthoNormalize(mModelLocal);

        if (mode == Mode::Local) {
            mModel = mModelLocal;
        } else {
            mModel = identity<Matrix4x4>();
            mModel[3] = matrix[3];
        }
        mModelSource = matrix;
        mModelScaleOrigin = {
            length(mModelSource[0]),
            length(mModelSource[1]),
            length(mModelSource[2]),
            0.0f
        };

        mModelInverse = inverse(mModel);
        mModelSourceInverse = inverse(mModelSource);
        mViewProjection = mProjectionMat * mViewMat;
        mMVP = mViewProjection * mModel;
        mMVPLocal = mViewProjection * mModelLocal;

        Matrix4x4 viewInverse = inverse(mViewMat);
        mCameraDir = viewInverse[2];
        mCameraEye = viewInverse[3];
        mCameraRight = viewInverse[0];
        mCameraUp = viewInverse[1];

        // projection reverse
        Vector4 nearPos = mProjectionMat * Vector4(0, 0, 1.f, 1.f);
        Vector4 farPos = mProjectionMat * Vector4(0, 0, 2.f, 1.f);

        mReversed = (nearPos.z() / nearPos.w()) > (farPos.z() / farPos.w());

        // compute scale from the size of camera right vector projected on screen at the matrix position
        Vector4 pointRight = imguizmo_transform_point(mViewProjection, viewInverse[0]);
        mScreenFactor = mGizmoSizeClipSpace / (pointRight.x() / pointRight.w() - mMVP[3].x() / mMVP[3].w());

        Vector4 rightViewInverse = mModelInverse * viewInverse[0];
        float rightLength = GetSegmentLengthClipSpace(mMVP, mDisplayRatio, Vector4{}, rightViewInverse);
        mScreenFactor = mGizmoSizeClipSpace / rightLength;

        ImVec2 centerSSpace = worldToPos(Vector4{}, mMVP);
        mScreenSquareCenter = centerSSpace;
        mScreenSquareMin = ImVec2(centerSSpace.x - 10.f, centerSSpace.y - 10.f);
        mScreenSquareMax = ImVec2(centerSSpace.x + 10.f, centerSSpace.y + 10.f);

        ComputeCameraRay(mRayOrigin, mRayVector);
    }

    ImVec2 worldToPos(
        const Vector4& worldPos,
        const Matrix4x4& mat) const
    {
        return ::worldToPos(worldPos, mat, ImVec2(mX, mY), ImVec2(mWidth, mHeight));
    }

    void ComputeCameraRay(Vector4& rayOrigin, Vector4& rayDir)
    {
        return ::ComputeCameraRay(
            mViewMat,
            mProjectionMat,
            mReversed,
            rayOrigin,
            rayDir,
            ImVec2(mX, mY),
            ImVec2(mWidth, mHeight)
        );
    }

    bool IsHoveringWindow() const
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGuiWindow* window = ImGui::FindWindowByName(mDrawList->_OwnerName);
        if (g.HoveredWindow == window) {
            // Mouse hovering drawlist window
            return true;
        }
        if (g.HoveredWindow != NULL) {
            // Any other window is hovered
            return false;
        }
        if (ImGui::IsMouseHoveringRect(window->InnerRect.Min, window->InnerRect.Max, false)) {
            // Hovering drawlist window rect, while no other window is hovered (for _NoInputs windows)
            return true;
        }
        return false;
    }

    bool HandleTranslation(
        Matrix4x4& matrix,
        Matrix4x4* deltaMatrix,
        Operation op,
        int& type,
        const osc::ui::GizmoOperationSnappingSteps* snap)
    {
        if (not(op & Operation::Translate) or type != MT_NONE) {
            return false;
        }
        const ImGuiIO& io = ImGui::GetIO();
        const bool applyRotationLocaly = mMode == Mode::Local or type == MT_MOVE_SCREEN;
        bool modified = false;

        // move
        if (mbUsing and (GetCurrentID() == mEditingID) and IsTranslateType(mCurrentOperation)) {
            ImGui::SetNextFrameWantCaptureMouse(true);

            const float signedLength = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlan);
            const float len = fabsf(signedLength); // near plan
            const Vector4 newPos = mRayOrigin + mRayVector * len;

            // compute delta
            const Vector4 newOrigin = newPos - mRelativeOrigin * mScreenFactor;
            Vector4 delta = newOrigin - mModel[3];

            // 1 axis constraint
            if (mCurrentOperation >= MT_MOVE_X and mCurrentOperation <= MT_MOVE_Z) {
                const int axisIndex = mCurrentOperation - MT_MOVE_X;
                const Vector4& axisValue = *(Vector4*)&mModel[axisIndex];
                const float lengthOnAxis = dot(axisValue, delta);
                delta = axisValue * lengthOnAxis;
            }

            // snap
            if (snap and snap->position) {
                Vector4 cumulativeDelta = mModel[3] + delta - mMatrixOrigin;
                if (applyRotationLocaly) {
                    Matrix4x4 modelSourceNormalized = mModelSource;
                    modelSourceNormalized[0] = normalize(modelSourceNormalized[0]);
                    modelSourceNormalized[1] = normalize(modelSourceNormalized[1]);
                    modelSourceNormalized[2] = normalize(modelSourceNormalized[2]);
                    Matrix4x4 modelSourceNormalizedInverse = inverse(modelSourceNormalized);
                    cumulativeDelta = imguizmo_transform_vector(modelSourceNormalizedInverse, cumulativeDelta);
                    ComputeSnap(cumulativeDelta, *snap->position);
                    cumulativeDelta = imguizmo_transform_vector(modelSourceNormalized, cumulativeDelta);
                } else {
                    ComputeSnap(cumulativeDelta, *snap->position);
                }
                delta = mMatrixOrigin + cumulativeDelta - mModel[3];
            }

            if (delta != mTranslationLastDelta) {
                modified = true;
            }
            mTranslationLastDelta = delta;

            // compute matrix & delta
            Matrix4x4 deltaMatrixTranslation = translate(identity<Matrix4x4>(), Vector3{delta});
            if (deltaMatrix) {
                *deltaMatrix = deltaMatrixTranslation;
            }

            const Matrix4x4 res = deltaMatrixTranslation * mModelSource;
            matrix = res;

            if (not io.MouseDown[0]) {
                mbUsing = false;
            }

            type = mCurrentOperation;
        } else {
            // find new possible way to move
            Vector4 gizmoHitProportion;
            type = mbOverGizmoHotspot ? MT_NONE : GetMoveType(op, &gizmoHitProportion);
            mbOverGizmoHotspot |= type != MT_NONE;
            if (type != MT_NONE) {
                ImGui::SetNextFrameWantCaptureMouse(true);
            }
            if (CanActivate() and type != MT_NONE) {
                mbUsing = true;
                mEditingID = GetCurrentID();
                mCurrentOperation = type;
                Vector4 movePlanNormal[] = {
                    mModel[0], mModel[1], mModel[2],
                    mModel[0], mModel[1], mModel[2],
                    -mCameraDir
                };

                Vector4 cameraToModelNormalized = normalize(mModel[3] - mCameraEye);
                for (unsigned int i = 0; i < 3; i++) {
                    Vector4 orthoVector = imguizmo_cross(movePlanNormal[i], cameraToModelNormalized);
                    movePlanNormal[i] = normalize(imguizmo_cross(movePlanNormal[i], orthoVector));
                }
                // pickup plan
                mTranslationPlan = BuildPlan(mModel[3], movePlanNormal[type - MT_MOVE_X]);
                const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlan);
                mTranslationPlanOrigin = mRayOrigin + mRayVector * len;
                mMatrixOrigin = mModel[3];

                mRelativeOrigin = (mTranslationPlanOrigin - mModel[3]) * (1.f / mScreenFactor);
            }
        }
        return modified;
    }

    bool HandleScale(
        Matrix4x4& matrix,
        Matrix4x4* deltaMatrix,
        Operation op,
        int& type,
        const osc::ui::GizmoOperationSnappingSteps *snap)
    {
        if ((not(op & Operation::Scale) and not(op & Operation::ScaleU))
            or type != MT_NONE
            or not mbMouseOver) {
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        bool modified = false;

        if (not mbUsing) {
            // find new possible way to scale
            type = mbOverGizmoHotspot ? MT_NONE : GetScaleType(op);
            mbOverGizmoHotspot |= type != MT_NONE;

            if (type != MT_NONE) {
                ImGui::SetNextFrameWantCaptureMouse(true);
            }
            if (CanActivate() and type != MT_NONE) {
                mbUsing = true;
                mEditingID = GetCurrentID();
                mCurrentOperation = type;
                const Vector4 movePlanNormal[] = {
                    mModelLocal[1], mModelLocal[2], mModelLocal[0], mModelLocal[2], mModelLocal[1], mModelLocal[0],
                    -mCameraDir
                };
                // pickup plan

                mTranslationPlan = BuildPlan(mModelLocal[3], movePlanNormal[type - MT_SCALE_X]);
                const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlan);
                mTranslationPlanOrigin = mRayOrigin + mRayVector * len;
                mMatrixOrigin = mModelLocal[3];
                mScale = Vector4{1.0f, 1.0f, 1.0f, 0.0f};
                mRelativeOrigin = (mTranslationPlanOrigin - mModelLocal[3]) * (1.f / mScreenFactor);
                mScaleValueOrigin = Vector4{
                    length(mModelSource[0]),
                    length(mModelSource[1]),
                    length(mModelSource[2]),
                    0.0f
                };
                mSaveMousePosx = io.MousePos.x;
            }
        }

        // scale
        if (mbUsing and (GetCurrentID() == mEditingID) and IsScaleType(mCurrentOperation)) {
            ImGui::SetNextFrameWantCaptureMouse(true);
            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlan);
            Vector4 newPos = mRayOrigin + mRayVector * len;
            Vector4 newOrigin = newPos - mRelativeOrigin * mScreenFactor;
            Vector4 delta = newOrigin - mModelLocal[3];

            // 1 axis constraint
            if (mCurrentOperation >= MT_SCALE_X and mCurrentOperation <= MT_SCALE_Z) {
                int axisIndex = mCurrentOperation - MT_SCALE_X;
                const Vector4 &axisValue = *(Vector4*) &mModelLocal[axisIndex];
                float lengthOnAxis = dot(axisValue, delta);
                delta = axisValue * lengthOnAxis;

                Vector4 baseVector = mTranslationPlanOrigin - mModelLocal[3];
                float ratio = dot(axisValue, baseVector + delta) / dot(axisValue, baseVector);

                mScale[axisIndex] = max(ratio, 0.001f);
            } else {
                float scaleDelta = (io.MousePos.x - mSaveMousePosx) * 0.01f;
                mScale = Vector4{max(1.f + scaleDelta, 0.001f)};
            }

            // snap
            if (snap and snap->scale) {
                ComputeSnap(mScale, *snap->scale);
            }

            // no 0 allowed
            for (int i = 0; i < 3; i++) {
                mScale[i] = max(mScale[i], 0.001f);
            }

            if (mScaleLast != mScale) {
                modified = true;
            }
            mScaleLast = mScale;

            // compute matrix & delta
            Matrix4x4 deltaMatrixScale = scale(identity<Matrix4x4>(), Vector3{mScale * mScaleValueOrigin});

            Matrix4x4 res = mModelLocal * deltaMatrixScale;
            matrix = res;

            if (deltaMatrix) {
                Vector4 deltaScale = mScale * mScaleValueOrigin;

                Vector4 originalScaleDivider;
                originalScaleDivider.x() = 1 / mModelScaleOrigin.x();
                originalScaleDivider.y() = 1 / mModelScaleOrigin.y();
                originalScaleDivider.z() = 1 / mModelScaleOrigin.z();

                deltaScale = deltaScale * originalScaleDivider;

                deltaMatrixScale = scale(identity<Matrix4x4>(), Vector3{deltaScale});
                *deltaMatrix = deltaMatrixScale;
            }

            if (not io.MouseDown[0]) {
                mbUsing = false;
                mScale = Vector4{1.0f, 1.0f, 1.0f, 0.0f};
            }

            type = mCurrentOperation;
        }
        return modified;
    }

    bool HandleRotation(
        Matrix4x4& matrix,
        Matrix4x4* deltaMatrix,
        Operation op,
        int& type,
        const osc::ui::GizmoOperationSnappingSteps* snap)
    {
        if (not(op & Operation::Rotate)
            or type != MT_NONE
            or not mbMouseOver) {
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        bool applyRotationLocaly = mMode == Mode::Local;
        bool modified = false;

        if (not mbUsing) {
            type = mbOverGizmoHotspot ? MT_NONE : GetRotateType(op);
            mbOverGizmoHotspot |= type != MT_NONE;

            if (type != MT_NONE) {
                ImGui::SetNextFrameWantCaptureMouse(true);
            }

            if (type == MT_ROTATE_SCREEN) {
                applyRotationLocaly = true;
            }

            if (CanActivate() and type != MT_NONE) {
                mbUsing = true;
                mEditingID = GetCurrentID();
                mCurrentOperation = type;
                const Vector4 rotatePlanNormal[] = {mModel[0], mModel[1], mModel[2], -mCameraDir};

                // pickup plan
                if (applyRotationLocaly) {
                    mTranslationPlan = BuildPlan(mModel[3], rotatePlanNormal[type - MT_ROTATE_X]);
                } else {
                    mTranslationPlan = BuildPlan(mModelSource[3], c_direction_unary[type - MT_ROTATE_X]);
                }

                const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlan);
                Vector4 localPos = mRayOrigin + mRayVector * len - mModel[3];
                mRotationVectorSource = normalize(localPos);
                mRotationAngleOrigin = ComputeAngleOnPlan();
            }
        }

        // rotation
        if (mbUsing and (GetCurrentID() == mEditingID) and IsRotateType(mCurrentOperation)) {
            ImGui::SetNextFrameWantCaptureMouse(true);

            mRotationAngle = ComputeAngleOnPlan();
            if (snap and snap->rotation) {
                static_assert(std::same_as<const Radians&, decltype(*snap->rotation)>);
                ComputeSnap(&mRotationAngle, snap->rotation->count());
            }
            Vector4 rotationAxisLocalSpace;

            rotationAxisLocalSpace = imguizmo_transform_vector(mModelInverse, Vector4{Vector3{mTranslationPlan}, 0.f});
            rotationAxisLocalSpace = normalize(rotationAxisLocalSpace);

            Matrix4x4 deltaRotation = matrix4x4_cast(angle_axis(Radians{mRotationAngle - mRotationAngleOrigin}, rotationAxisLocalSpace));
            if (mRotationAngle != mRotationAngleOrigin) {
                modified = true;
            }
            mRotationAngleOrigin = mRotationAngle;

            Matrix4x4 scaleOrigin = scale(identity<Matrix4x4>(), Vector3{mModelScaleOrigin});

            if (applyRotationLocaly) {
                matrix = mModelLocal * deltaRotation * scaleOrigin;
            } else {
                Matrix4x4 res = mModelSource;
                res[3] = {};

                matrix = deltaRotation * res;
                matrix[3] = mModelSource[3];
            }

            if (deltaMatrix) {
                *deltaMatrix = mModel * deltaRotation * mModelInverse;
            }

            if (not io.MouseDown[0]) {
                mbUsing = false;
                mEditingID = blank_id();
            }
            type = mCurrentOperation;
        }
        return modified;
    }

    float ComputeAngleOnPlan() const
    {
        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlan);
        Vector4 localPos = normalize(mRayOrigin + mRayVector * len - mModel[3]);

        const Vector4 perpendicularVector = normalize(imguizmo_cross(mRotationVectorSource, mTranslationPlan));
        float acosAngle = clamp(dot(localPos, mRotationVectorSource), -1.f, 1.f);
        float angle = acosf(acosAngle);
        angle *= (dot(localPos, perpendicularVector) < 0.f) ? 1.f : -1.f;
        return angle;
    }

    void HandleAndDrawLocalBounds(
        const float* bounds,
        Matrix4x4* matrix,
        const float* snapValues,
        Operation operation)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* drawList = mDrawList;

        // compute best projection axis
        Vector4 axesWorldDirections[3];
        Vector4 bestAxisWorldDirection = {0.0f, 0.0f, 0.0f, 0.0f};
        int axes[3];
        unsigned int numAxes = 1;
        axes[0] = mBoundsBestAxis;
        int bestAxis = axes[0];
        if (not mbUsingBounds) {
            numAxes = 0;
            float bestDot = 0.f;
            for (int i = 0; i < 3; i++) {
                Vector4 dirPlaneNormalWorld = normalize(imguizmo_transform_vector(mModelSource, c_direction_unary[i]));

                float dt = fabsf(dot(normalize(mCameraEye - mModelSource[3]), dirPlaneNormalWorld));
                if (dt >= bestDot) {
                    bestDot = dt;
                    bestAxis = i;
                    bestAxisWorldDirection = dirPlaneNormalWorld;
                }

                if (dt >= 0.1f) {
                    axes[numAxes] = i;
                    axesWorldDirections[numAxes] = dirPlaneNormalWorld;
                    ++numAxes;
                }
            }
        }

        if (numAxes == 0) {
            axes[0] = bestAxis;
            axesWorldDirections[0] = bestAxisWorldDirection;
            numAxes = 1;
        } else if (bestAxis != axes[0]) {
            unsigned int bestIndex = 0;
            for (unsigned int i = 0; i < numAxes; i++) {
                if (axes[i] == bestAxis) {
                    bestIndex = i;
                    break;
                }
            }
            int tempAxis = axes[0];
            axes[0] = axes[bestIndex];
            axes[bestIndex] = tempAxis;
            Vector4 tempDirection = axesWorldDirections[0];
            axesWorldDirections[0] = axesWorldDirections[bestIndex];
            axesWorldDirections[bestIndex] = tempDirection;
        }

        for (unsigned int axisIndex = 0; axisIndex < numAxes; ++axisIndex) {
            bestAxis = axes[axisIndex];
            bestAxisWorldDirection = axesWorldDirections[axisIndex];

            // corners
            Vector4 aabb[4];

            int secondAxis = (bestAxis + 1) % 3;
            int thirdAxis = (bestAxis + 2) % 3;

            for (int i = 0; i < 4; i++) {
                aabb[i][3] = aabb[i][bestAxis] = 0.f;
                aabb[i][secondAxis] = bounds[secondAxis + 3 * (i >> 1)];
                aabb[i][thirdAxis] = bounds[thirdAxis + 3 * ((i >> 1) ^ (i & 1))];
            }

            // draw bounds
            unsigned int anchorAlpha = mbEnable ? IM_COL32_BLACK : IM_COL32(0, 0, 0, 0x80);

            Matrix4x4 boundsMVP = mViewProjection * mModelSource;
            for (int i = 0; i < 4; i++) {
                ImVec2 worldBound1 = worldToPos(aabb[i], boundsMVP);
                ImVec2 worldBound2 = worldToPos(aabb[(i + 1) % 4], boundsMVP);
                if (not IsInContextRect(worldBound1) or not IsInContextRect(worldBound2)) {
                    continue;
                }
                float boundDistance = sqrtf(ImLengthSqr(worldBound1 - worldBound2));
                int stepCount = (int) (boundDistance / 10.f);
                stepCount = min(stepCount, 1000);
                for (int j = 0; j < stepCount; j++) {
                    float stepLength = 1.f / (float) stepCount;
                    float t1 = (float) j * stepLength;
                    float t2 = (float) j * stepLength + stepLength * 0.5f;
                    ImVec2 worldBoundSS1 = ImLerp(worldBound1, worldBound2, ImVec2(t1, t1));
                    ImVec2 worldBoundSS2 = ImLerp(worldBound1, worldBound2, ImVec2(t2, t2));
                    //drawList->AddLine(worldBoundSS1, worldBoundSS2, IM_COL32(0, 0, 0, 0) + anchorAlpha, 3.f);
                    drawList->AddLine(worldBoundSS1, worldBoundSS2, IM_COL32(0xAA, 0xAA, 0xAA, 0) + anchorAlpha, 2.f);
                }
                Vector4 midPoint = (aabb[i] + aabb[(i + 1) % 4]) * 0.5f;
                ImVec2 midBound = worldToPos(midPoint, boundsMVP);
                constexpr float AnchorBigRadius = 8.f;
                constexpr float AnchorSmallRadius = 6.f;
                bool overBigAnchor = ImLengthSqr(worldBound1 - io.MousePos) <= (AnchorBigRadius * AnchorBigRadius);
                bool overSmallAnchor = ImLengthSqr(midBound - io.MousePos) <= (AnchorBigRadius * AnchorBigRadius);

                int type = MT_NONE;
                Vector4 gizmoHitProportion;

                if (operation & Operation::Translate) {
                    type = GetMoveType(operation, &gizmoHitProportion);
                }
                if ((operation & Operation::Rotate) and type == MT_NONE) {
                    type = GetRotateType(operation);
                }
                if ((operation & Operation::Scale) and type == MT_NONE) {
                    type = GetScaleType(operation);
                }

                if (type != MT_NONE) {
                    overBigAnchor = false;
                    overSmallAnchor = false;
                }

                ImU32 selectionColor = GetColorU32(SELECTION);

                unsigned int bigAnchorColor = overBigAnchor ? selectionColor : (IM_COL32(0xAA, 0xAA, 0xAA, 0) + anchorAlpha);
                unsigned int smallAnchorColor = overSmallAnchor ? selectionColor : (IM_COL32(0xAA, 0xAA, 0xAA, 0) + anchorAlpha);

                drawList->AddCircleFilled(worldBound1, AnchorBigRadius, IM_COL32_BLACK);
                drawList->AddCircleFilled(worldBound1, AnchorBigRadius - 1.2f, bigAnchorColor);

                drawList->AddCircleFilled(midBound, AnchorSmallRadius, IM_COL32_BLACK);
                drawList->AddCircleFilled(midBound, AnchorSmallRadius - 1.2f, smallAnchorColor);
                int oppositeIndex = (i + 2) % 4;
                // big anchor on corners
                if (not mbUsingBounds and mbEnable and overBigAnchor and CanActivate()) {
                    mBoundsPivot = imguizmo_transform_point(mModelSource, aabb[(i + 2) % 4]);
                    mBoundsAnchor = imguizmo_transform_point(mModelSource, aabb[i]);
                    mBoundsPlan = BuildPlan(mBoundsAnchor, bestAxisWorldDirection);
                    mBoundsBestAxis = bestAxis;
                    mBoundsAxis[0] = secondAxis;
                    mBoundsAxis[1] = thirdAxis;

                    mBoundsLocalPivot = {};
                    mBoundsLocalPivot[secondAxis] = aabb[oppositeIndex][secondAxis];
                    mBoundsLocalPivot[thirdAxis] = aabb[oppositeIndex][thirdAxis];

                    mbUsingBounds = true;
                    mEditingID = GetCurrentID();
                    mBoundsMatrix = mModelSource;
                }
                // small anchor on middle of segment
                if (not mbUsingBounds and mbEnable and overSmallAnchor && CanActivate()) {
                    Vector4 midPointOpposite = (aabb[(i + 2) % 4] + aabb[(i + 3) % 4]) * 0.5f;
                    mBoundsPivot = imguizmo_transform_point(mModelSource, midPointOpposite);
                    mBoundsAnchor = imguizmo_transform_point(mModelSource, midPoint);
                    mBoundsPlan = BuildPlan(mBoundsAnchor, bestAxisWorldDirection);
                    mBoundsBestAxis = bestAxis;
                    int indices[] = {secondAxis, thirdAxis};
                    mBoundsAxis[0] = indices[i % 2];
                    mBoundsAxis[1] = -1;

                    mBoundsLocalPivot = {};
                    mBoundsLocalPivot[mBoundsAxis[0]] = aabb[oppositeIndex][indices[i % 2]];
                    // bounds[context.mBoundsAxis[0]] * (((i + 1) & 2) ? 1.f : -1.f);

                    mbUsingBounds = true;
                    mEditingID = GetCurrentID();
                    mBoundsMatrix = mModelSource;
                }
            }

            if (mbUsingBounds and (GetCurrentID() == mEditingID)) {
                Matrix4x4 scale = identity<Matrix4x4>();

                // compute projected mouse position on plan
                const float len = IntersectRayPlane(mRayOrigin, mRayVector, mBoundsPlan);
                Vector4 newPos = mRayOrigin + mRayVector * len;

                // compute a reference and delta vectors base on mouse move
                Vector4 deltaVector = abs(newPos - mBoundsPivot);
                Vector4 referenceVector = abs(mBoundsAnchor - mBoundsPivot);

                // for 1 or 2 axes, compute a ratio that's used for scale and snap it based on resulting length
                for (int i = 0; i < 2; i++) {
                    int axisIndex1 = mBoundsAxis[i];
                    if (axisIndex1 == -1) {
                        continue;
                    }

                    float ratioAxis = 1.f;
                    Vector4 axisDir = abs(mBoundsMatrix[axisIndex1]);

                    float dtAxis = dot(axisDir, referenceVector);
                    float boundSize = bounds[axisIndex1 + 3] - bounds[axisIndex1];
                    if (dtAxis > FLT_EPSILON) {
                        ratioAxis = dot(axisDir, deltaVector) / dtAxis;
                    }

                    if (snapValues) {
                        float length = boundSize * ratioAxis;
                        ComputeSnap(&length, snapValues[axisIndex1]);
                        if (boundSize > FLT_EPSILON) {
                            ratioAxis = length / boundSize;
                        }
                    }
                    scale[axisIndex1] *= ratioAxis;
                }

                // transform matrix
                Matrix4x4 postScale = translate(identity<Matrix4x4>(), Vector3{mBoundsLocalPivot});
                Matrix4x4 preScale = translate(identity<Matrix4x4>(), Vector3{-mBoundsLocalPivot});
                Matrix4x4 res = mBoundsMatrix * postScale * scale * preScale;
                *matrix = res;

                // info text
                char tmps[512];
                ImVec2 destinationPosOnScreen = worldToPos(mModel[3], mViewProjection);
                ImFormatString(tmps, sizeof(tmps), "X: %.2f Y: %.2f Z: %.2f"
                               , (bounds[3] - bounds[0]) * length(mBoundsMatrix[0]) * length(scale[0])
                               , (bounds[4] - bounds[1]) * length(mBoundsMatrix[1]) * length(scale[1])
                               , (bounds[5] - bounds[2]) * length(mBoundsMatrix[2]) * length(scale[2])
                );
                drawList->AddText(
                    ImVec2(destinationPosOnScreen.x + AnnotationOffset() + 1.0f, destinationPosOnScreen.y + AnnotationOffset() + 1.0f),
                    GetColorU32(TEXT_SHADOW),
                    tmps
                );
                drawList->AddText(
                    ImVec2(destinationPosOnScreen.x + AnnotationOffset(), destinationPosOnScreen.y + AnnotationOffset()),
                    GetColorU32(TEXT),
                    tmps
                );
            }

            if (not io.MouseDown[0]) {
                mbUsingBounds = false;
                mEditingID = blank_id();
            }
            if (mbUsingBounds) {
                break;
            }
        }
    }

    ImU32 GetColorU32(int idx) const
    {
        IM_ASSERT(idx < COLOR::COUNT);
        return ImGui::ColorConvertFloat4ToU32(mStyle.Colors[idx]);
    }

    bool IsInContextRect(ImVec2 p) const
    {
        return IsWithin(p.x, mX, mXMax) and IsWithin(p.y, mY, mYMax);
    }

    void ComputeColors(ImU32* colors, int type, Operation operation) const
    {
        if (mbEnable) {
            ImU32 selectionColor = GetColorU32(SELECTION);

            switch (operation) {
                case Operation::Translate:
                    colors[0] = (type == MT_MOVE_SCREEN) ? selectionColor : ImGui::ColorConvertFloat4ToU32({0.8f, 0.5f, 0.3f, 0.8f});

                    // HACK: osc: make translation circle orange, which stands out from the mostly-white geometry
                    for (int i = 0; i < 3; i++) {
                        colors[i + 1] = (type == (int) (MT_MOVE_X + i)) ? selectionColor : GetColorU32(DIRECTION_X + i);
                        colors[i + 4] = (type == (int) (MT_MOVE_YZ + i)) ? selectionColor : GetColorU32(PLANE_X + i);
                        colors[i + 4] = (type == MT_MOVE_SCREEN) ? selectionColor : colors[i + 4];
                    }
                    break;
                case Operation::Rotate:
                    colors[0] = (type == MT_ROTATE_SCREEN) ? selectionColor : IM_COL32_WHITE;
                    for (int i = 0; i < 3; i++) {
                        colors[i + 1] = (type == (int) (MT_ROTATE_X + i)) ? selectionColor : GetColorU32(DIRECTION_X + i);
                    }
                    break;
                case Operation::ScaleU:
                case Operation::Scale:
                    colors[0] = (type == MT_SCALE_XYZ) ? selectionColor : IM_COL32_WHITE;
                    for (int i = 0; i < 3; i++) {
                        colors[i + 1] = (type == (int) (MT_SCALE_X + i)) ? selectionColor : GetColorU32(DIRECTION_X + i);
                    }
                    break;
                // note: this internal function is only called with three possible values for operation
                default:
                    break;
            }
        } else {
            ImU32 inactiveColor = GetColorU32(INACTIVE);
            for (int i = 0; i < 7; i++) {
                colors[i] = inactiveColor;
            }
        }
    }

    void DrawRotationGizmo(Operation op, int type)
    {
        if (not(op & Operation::Rotate)) {
            return;
        }
        ImDrawList* drawList = mDrawList;

        bool isMultipleAxesMasked = mAxisMask & (mAxisMask - 1);
        bool isNoAxesMasked = not mAxisMask;

        // colors
        ImU32 colors[7];
        ComputeColors(colors, type, Operation::Rotate);

        Vector4 cameraToModelNormalized;
        if (mIsOrthographic) {
            Matrix4x4 viewInverse = inverse(mViewMat);
            cameraToModelNormalized = -viewInverse[2];
        } else {
            cameraToModelNormalized = normalize(mModel[3] - mCameraEye);
        }

        cameraToModelNormalized = imguizmo_transform_vector(mModelInverse, cameraToModelNormalized);

        mRadiusSquareCenter = c_screen_rotate_size * mHeight;

        bool hasRSC = op & Operation::RotateInScreen;
        for (int axis = 0; axis < 3; axis++) {
            if (not(op & (Operation::RotateZ >> axis))) {
                continue;
            }

            bool isAxisMasked = (1 << (2 - axis)) & mAxisMask;

            if ((not isAxisMasked or isMultipleAxesMasked) and not isNoAxesMasked) {
                continue;
            }
            const bool usingAxis = (mbUsing && type == MT_ROTATE_Z - axis);
            const int circleMul = (hasRSC && !usingAxis) ? 1 : 2;

            std::vector<ImVec2> circlePositions;
            circlePositions.reserve(circleMul * c_half_circle_segment_count + 1);
            ImVec2 *circlePos = circlePositions.data();

            float angleStart = atan2f(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + pi_v<float> * 0.5f;

            for (int i = 0; i < circleMul * c_half_circle_segment_count + 1; i++) {
                float ng = angleStart + (float) circleMul * pi_v<float> * ((float) i / (float) (circleMul * c_half_circle_segment_count));
                Vector4 axisPos = {cosf(ng), sinf(ng), 0.f, 0.f};
                Vector4 pos = Vector4{axisPos[axis], axisPos[(axis + 1) % 3], axisPos[(axis + 2) % 3], 0.0f} * mScreenFactor * c_rotation_display_factor;
                circlePos[i] = worldToPos(pos, mMVP);
            }
            if (not mbUsing or usingAxis) {
                drawList->AddPolyline(
                    circlePos,
                    circleMul * c_half_circle_segment_count + 1, colors[3 - axis],
                    false,
                    mStyle.RotationLineThickness
                );
            }

            float radiusAxis = sqrtf((ImLengthSqr(worldToPos(mModel[3], mViewProjection) - circlePos[0])));
            if (radiusAxis > mRadiusSquareCenter) {
                mRadiusSquareCenter = radiusAxis;
            }
        }
        if (hasRSC && (not mbUsing or type == MT_ROTATE_SCREEN) and (not isMultipleAxesMasked and isNoAxesMasked)) {
            drawList->AddCircle(
                worldToPos(mModel[3], mViewProjection),
                mRadiusSquareCenter,
                colors[0],
                64,
                mStyle.RotationOuterLineThickness
            );
        }

        if (mbUsing and (GetCurrentID() == mEditingID) and IsRotateType(type)) {
            ImVec2 circlePos[c_half_circle_segment_count + 1];

            circlePos[0] = worldToPos(mModel[3], mViewProjection);
            for (unsigned int i = 1; i < c_half_circle_segment_count + 1; i++) {
                float ng = mRotationAngle * ((float) (i - 1) / (float) (c_half_circle_segment_count - 1));
                Matrix4x4 rotateVectorMatrix = matrix4x4_cast(angle_axis(Radians{ng}, Vector3{mTranslationPlan}));
                Vector4 pos = imguizmo_transform_point(rotateVectorMatrix, mRotationVectorSource);
                pos *= mScreenFactor * c_rotation_display_factor;
                circlePos[i] = worldToPos(pos + mModel[3], mViewProjection);
            }
            drawList->AddConvexPolyFilled(
                circlePos,
                c_half_circle_segment_count + 1,
                GetColorU32(ROTATION_USING_FILL)
            );
            drawList->AddPolyline(
                circlePos,
                c_half_circle_segment_count + 1,
                GetColorU32(ROTATION_USING_BORDER),
                true,
                mStyle.RotationLineThickness
            );

            ImVec2 destinationPosOnScreen = circlePos[1];
            char tmps[512];
            ImFormatString(
                tmps,
                sizeof(tmps),
                c_rotation_info_mask[type - MT_ROTATE_X].c_str(),
                (mRotationAngle / pi_v<float>) * 180.f,
                mRotationAngle
            );
            drawList->AddText(
                ImVec2(destinationPosOnScreen.x + AnnotationOffset() + 1.0f, destinationPosOnScreen.y + AnnotationOffset() + 1.0f),
                GetColorU32(TEXT_SHADOW),
                tmps
            );
            drawList->AddText(
                ImVec2(destinationPosOnScreen.x + AnnotationOffset(), destinationPosOnScreen.y + AnnotationOffset()),
                GetColorU32(TEXT),
                tmps
            );
        }
    }

    void DrawHatchedAxis(const Vector4 &axis) const
    {
        if (mStyle.HatchedAxisLineThickness <= 0.0f) {
            return;
        }

        for (int j = 1; j < 10; j++) {
            ImVec2 baseSSpace2 = worldToPos(axis * 0.05f * (float) (j * 2) * mScreenFactor, mMVP);
            ImVec2 worldDirSSpace2 = worldToPos(axis * 0.05f * (float) (j * 2 + 1) * mScreenFactor, mMVP);
            mDrawList->AddLine(
                baseSSpace2,
                worldDirSSpace2,
                GetColorU32(HATCHED_AXIS_LINES),
                mStyle.HatchedAxisLineThickness
            );
        }
    }

    void DrawScaleGizmo(Operation op, int type)
    {
        ImDrawList* drawList = mDrawList;

        if (not(op & Operation::Scale)) {
            return;
        }

        // colors
        ImU32 colors[7];
        ComputeColors(colors, type, Operation::Scale);

        // draw
        Vector4 scaleDisplay = {1.f, 1.f, 1.f, 1.f};

        if (mbUsing and (GetCurrentID() == mEditingID)) {
            scaleDisplay = mScale;
        }

        for (int i = 0; i < 3; i++) {
            if (not(op & (Operation::ScaleX << i))) {
                continue;
            }

            const bool usingAxis = (mbUsing && type == MT_SCALE_X + i);
            if (not mbUsing or usingAxis) {
                Vector4 dirPlaneX, dirPlaneY, dirAxis;
                bool belowAxisLimit, belowPlaneLimit;
                ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);

                // draw axis
                if (belowAxisLimit) {
                    bool hasTranslateOnAxis = Contains(op, Operation::TranslateX << i);
                    float markerScale = hasTranslateOnAxis ? 1.4f : 1.0f;
                    ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * mScreenFactor, mMVP);
                    ImVec2 worldDirSSpaceNoScale = worldToPos(dirAxis * markerScale * mScreenFactor, mMVP);
                    ImVec2 worldDirSSpace = worldToPos((dirAxis * markerScale * scaleDisplay[i]) * mScreenFactor, mMVP);

                    if (mbUsing and (GetCurrentID() == mEditingID)) {
                        ImU32 scaleLineColor = GetColorU32(SCALE_LINE);
                        drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, scaleLineColor, mStyle.ScaleLineThickness);
                        drawList->AddCircleFilled(worldDirSSpaceNoScale, mStyle.ScaleLineCircleSize, scaleLineColor);
                    }

                    if (not hasTranslateOnAxis or mbUsing) {
                        drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], mStyle.ScaleLineThickness);
                    }
                    drawList->AddCircleFilled(worldDirSSpace, mStyle.ScaleLineCircleSize, colors[i + 1]);

                    if (mAxisFactor[i] < 0.f) {
                        DrawHatchedAxis(dirAxis * scaleDisplay[i]);
                    }
                }
            }
        }

        // draw screen cirle
        drawList->AddCircleFilled(mScreenSquareCenter, mStyle.CenterCircleSize, colors[0], 32);

        if (mbUsing and (GetCurrentID() == mEditingID) and IsScaleType(type)) {
            //ImVec2 sourcePosOnScreen = worldToPos(context.mMatrixOrigin, context.mViewProjection);
            ImVec2 destinationPosOnScreen = worldToPos(mModel[3], mViewProjection);
            /*Vector4 dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y);
            dif.Normalize();
            dif *= 5.f;
            drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
            drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
            drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);
            */
            char tmps[512];
            //Vector4 deltaInfo = context.mModel.v.position - context.mMatrixOrigin;
            int componentInfoIndex = (type - MT_SCALE_X) * 3;
            ImFormatString(
                tmps,
                sizeof(tmps),
                c_scale_info_mask[type - MT_SCALE_X].c_str(),
                scaleDisplay[c_translation_info_index[componentInfoIndex]]
            );
            drawList->AddText(
                ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15),
                GetColorU32(TEXT_SHADOW),
                tmps
            );
            drawList->AddText(
                ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14),
                GetColorU32(TEXT),
                tmps
            );
        }
    }

    void DrawScaleUniveralGizmo(Operation op, int type)
    {
        ImDrawList* drawList = mDrawList;

        if (not(op & Operation::ScaleU)) {
            return;
        }

        // colors
        ImU32 colors[7];
        ComputeColors(colors, type, Operation::ScaleU);

        // draw
        Vector4 scaleDisplay = {1.f, 1.f, 1.f, 1.f};

        if (mbUsing and (GetCurrentID() == mEditingID)) {
            scaleDisplay = mScale;
        }

        for (int i = 0; i < 3; i++) {
            if (not(op & (Operation::ScaleXU << i))) {
                continue;
            }
            const bool usingAxis = (mbUsing && type == MT_SCALE_X + i);
            if (not mbUsing or usingAxis) {
                Vector4 dirPlaneX, dirPlaneY, dirAxis;
                bool belowAxisLimit, belowPlaneLimit;
                ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);

                // draw axis
                if (belowAxisLimit) {
                    bool hasTranslateOnAxis = Contains(op, Operation::TranslateX << i);
                    float markerScale = hasTranslateOnAxis ? 1.4f : 1.0f;
                    //ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * context.mScreenFactor, context.mMVPLocal);
                    //ImVec2 worldDirSSpaceNoScale = worldToPos(dirAxis * markerScale * context.mScreenFactor, context.mMVP);
                    ImVec2 worldDirSSpace = worldToPos((dirAxis * markerScale * scaleDisplay[i]) * mScreenFactor, mMVPLocal);

#if 0
                    if (context.mbUsing && (context.GetCurrentID() == context.mEditingID)) {
                        drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, IM_COL32(0x40, 0x40, 0x40, 0xFF), 3.f);
                        drawList->AddCircleFilled(worldDirSSpaceNoScale, 6.f, IM_COL32(0x40, 0x40, 0x40, 0xFF));
                    }
                    /*
                    if (!hasTranslateOnAxis || context.mbUsing)
                    {
                       drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 3.f);
                    }
                    */
#endif
                    drawList->AddCircleFilled(worldDirSSpace, 12.f, colors[i + 1]);
                }
            }
        }

        // draw screen cirle
        drawList->AddCircle(mScreenSquareCenter, 20.f, colors[0], 32, mStyle.CenterCircleSize);

        if (mbUsing and (GetCurrentID() == mEditingID) and IsScaleType(type)) {
            //ImVec2 sourcePosOnScreen = worldToPos(context.mMatrixOrigin, context.mViewProjection);
            ImVec2 destinationPosOnScreen = worldToPos(mModel[3], mViewProjection);
            /*Vector4 dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y);
            dif.Normalize();
            dif *= 5.f;
            drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
            drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
            drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);
            */
            char tmps[512];
            //Vector4 deltaInfo = context.mModel.v.position - context.mMatrixOrigin;
            int componentInfoIndex = (type - MT_SCALE_X) * 3;
            ImFormatString(tmps, sizeof(tmps), c_scale_info_mask[type - MT_SCALE_X].c_str(),
                           scaleDisplay[c_translation_info_index[componentInfoIndex]]);
            drawList->AddText(
                ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15),
                GetColorU32(TEXT_SHADOW),
                tmps
            );
            drawList->AddText(
                ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14),
                GetColorU32(TEXT),
                tmps
            );
        }
    }

    void DrawTranslationGizmo(Operation op, int type)
    {
        ImDrawList* drawList = mDrawList;
        if (not drawList) {
            return;
        }

        if (not(op & Operation::Translate)) {
            return;
        }

        // colors
        ImU32 colors[7];
        ComputeColors(colors, type, Operation::Translate);

        const ImVec2 origin = worldToPos(mModel[3], mViewProjection);

        // draw
        bool belowAxisLimit = false;
        bool belowPlaneLimit = false;
        for (int i = 0; i < 3; ++i) {
            Vector4 dirPlaneX, dirPlaneY, dirAxis;
            ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

            if (not mbUsing or (mbUsing && type == MT_MOVE_X + i)) {
                // draw axis
                if (belowAxisLimit && (op & (Operation::TranslateX << i))) {
                    ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * mScreenFactor, mMVP);
                    ImVec2 worldDirSSpace = worldToPos(dirAxis * mScreenFactor, mMVP);

                    drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], mStyle.TranslationLineThickness);

                    // Arrow head begin
                    ImVec2 dir(origin - worldDirSSpace);

                    float d = sqrtf(ImLengthSqr(dir));
                    dir /= d; // Normalize
                    dir *= mStyle.TranslationLineArrowSize;

                    ImVec2 ortogonalDir(dir.y, -dir.x); // Perpendicular vector
                    ImVec2 a(worldDirSSpace + dir);
                    drawList->AddTriangleFilled(
                        worldDirSSpace - dir,
                        a + ortogonalDir,
                        a - ortogonalDir,
                        colors[i + 1]
                    );
                    // Arrow head end

                    if (mAxisFactor[i] < 0.f) {
                        DrawHatchedAxis(dirAxis);
                    }
                }
            }
            // draw plane
            if (not mbUsing or (mbUsing and type == MT_MOVE_YZ + i)) {
                if (belowPlaneLimit and Contains(op, TRANSLATE_PLANS[i])) {
                    ImVec2 screenQuadPts[4];
                    for (int j = 0; j < 4; ++j) {
                        Vector4 cornerWorldPos = (dirPlaneX * c_quad_uv[j * 2] + dirPlaneY * c_quad_uv[j * 2 + 1]) * mScreenFactor;
                        screenQuadPts[j] = worldToPos(cornerWorldPos, mMVP);
                    }
                    drawList->AddPolyline(screenQuadPts, 4, GetColorU32(DIRECTION_X + i), true, 1.0f);
                    drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[i + 4]);
                }
            }
        }

        drawList->AddCircleFilled(mScreenSquareCenter, mStyle.CenterCircleSize, colors[0], 32);

        if (mbUsing and (GetCurrentID() == mEditingID) && IsTranslateType(type)) {
            ImU32 translationLineColor = GetColorU32(TRANSLATION_LINE);

            ImVec2 sourcePosOnScreen = worldToPos(mMatrixOrigin, mViewProjection);
            ImVec2 destinationPosOnScreen = worldToPos(mModel[3], mViewProjection);
            Vector4 dif = {
                destinationPosOnScreen.x - sourcePosOnScreen.x,
                destinationPosOnScreen.y - sourcePosOnScreen.y,
                0.f,
                0.f
            };
            dif = normalize(dif);
            dif *= 5.f;
            drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
            drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
            drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x(), sourcePosOnScreen.y + dif.y()),
                              ImVec2(destinationPosOnScreen.x - dif.x(), destinationPosOnScreen.y - dif.y()),
                              translationLineColor, 2.f);

            char tmps[512];
            Vector4 deltaInfo = mModel[3] - mMatrixOrigin;
            int componentInfoIndex = (type - MT_MOVE_X) * 3;
            ImFormatString(
                tmps,
                sizeof(tmps),
                c_translation_info_mask[type - MT_MOVE_X].c_str(),
                deltaInfo[c_translation_info_index[componentInfoIndex]],
                deltaInfo[c_translation_info_index[componentInfoIndex + 1]],
                deltaInfo[c_translation_info_index[componentInfoIndex + 2]]
            );
            drawList->AddText(
                ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15),
                GetColorU32(TEXT_SHADOW),
                tmps
            );
            drawList->AddText(
                ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14),
                GetColorU32(TEXT),
                tmps
            );
        }
    }

    ImDrawList* mDrawList;
    Style mStyle;

    Mode mMode;
    Matrix4x4 mViewMat;
    Matrix4x4 mProjectionMat;
    Matrix4x4 mModel;
    Matrix4x4 mModelLocal; // orthonormalized model
    Matrix4x4 mModelInverse;
    Matrix4x4 mModelSource;
    Matrix4x4 mModelSourceInverse;
    Matrix4x4 mMVP;
    Matrix4x4 mMVPLocal; // MVP with full model matrix whereas mMVP's model matrix might only be translation in case of World space edition
    Matrix4x4 mViewProjection;

    Vector4 mModelScaleOrigin;
    Vector4 mCameraEye;
    Vector4 mCameraRight;
    Vector4 mCameraDir;
    Vector4 mCameraUp;
    Vector4 mRayOrigin;
    Vector4 mRayVector;

    float  mRadiusSquareCenter;
    ImVec2 mScreenSquareCenter;
    ImVec2 mScreenSquareMin;
    ImVec2 mScreenSquareMax;

    float mScreenFactor;
    Vector4 mRelativeOrigin;

    bool mbUsing = false;
    bool mbEnable = true;
    bool mbMouseOver;
    bool mReversed; // reversed projection matrix

    // translation
    Vector4 mTranslationPlan;
    Vector4 mTranslationPlanOrigin;
    Vector4 mMatrixOrigin;
    Vector4 mTranslationLastDelta;

    // rotation
    Vector4 mRotationVectorSource;
    float mRotationAngle;
    float mRotationAngleOrigin;
    //Vector4 mWorldToLocalAxis;

    // scale
    Vector4 mScale;
    Vector4 mScaleValueOrigin;
    Vector4 mScaleLast;
    float mSaveMousePosx;

    // save axis factor when using gizmo
    bool mBelowAxisLimit[3];
    int mAxisMask = 0;
    bool mBelowPlaneLimit[3];
    float mAxisFactor[3];

    float mAxisLimit=0.0025f;
    float mPlaneLimit=0.02f;

    // bounds stretching
    Vector4 mBoundsPivot;
    Vector4 mBoundsAnchor;
    Vector4 mBoundsPlan;
    Vector4 mBoundsLocalPivot;
    int mBoundsBestAxis;
    int mBoundsAxis[2];
    bool mbUsingBounds = false;
    Matrix4x4 mBoundsMatrix;

    //
    int mCurrentOperation;

    float mX = 0.f;
    float mY = 0.f;
    float mWidth = 0.f;
    float mHeight = 0.f;
    float mXMax = 0.f;
    float mYMax = 0.f;
    float mDisplayRatio = 1.f;

    bool mIsOrthographic = false;
    // check to not have multiple gizmo highlighted at the same time
    bool mbOverGizmoHotspot = false;

    ImVector<ImGuiID> mIDStack;
    ImGuiID mEditingID = blank_id();
    Operation mOperation = Operation::None;

    bool mAllowAxisFlip = false;
    float mGizmoSizeClipSpace = 0.1f;
};

osc::ui::gizmo::detail::GizmoContext::GizmoContext() : impl_{std::make_unique<Impl>()} {}
osc::ui::gizmo::detail::GizmoContext::GizmoContext(GizmoContext&&) noexcept = default;
osc::ui::gizmo::detail::GizmoContext::~GizmoContext() noexcept = default;
GizmoContext& osc::ui::gizmo::detail::GizmoContext::operator=(GizmoContext&&) noexcept = default;
void osc::ui::gizmo::detail::GizmoContext::SetDrawlist(ImDrawList *drawlist) { impl_->SetDrawlist(drawlist); }
void osc::ui::gizmo::detail::GizmoContext::BeginFrame() { impl_->BeginFrame(); }
bool osc::ui::gizmo::detail::GizmoContext::IsOver() { return impl_->IsOver(); }
bool osc::ui::gizmo::detail::GizmoContext::IsOver(Operation op) { return impl_->IsOver(op); }
bool osc::ui::gizmo::detail::GizmoContext::IsUsing() { return impl_->IsUsing(); }
bool osc::ui::gizmo::detail::GizmoContext::IsUsingAny() { return impl_->IsUsingAny(); }
void osc::ui::gizmo::detail::GizmoContext::Enable(bool enable) { impl_->Enable(enable); }
void osc::ui::gizmo::detail::GizmoContext::SetRect(const Rect& ui_rect) { impl_->SetRect(ui_rect); }
void osc::ui::gizmo::detail::GizmoContext::SetOrthographic(bool isOrthographic) { impl_->SetOrthographic(isOrthographic); }
void osc::ui::gizmo::detail::GizmoContext::PushID(UID uid) { impl_->PushID(uid); }
void osc::ui::gizmo::detail::GizmoContext::PopID() { impl_->PopID(); }
void osc::ui::gizmo::detail::GizmoContext::SetGizmoSizeClipSpace(float value) { impl_->SetGizmoSizeClipSpace(value); }
void osc::ui::gizmo::detail::GizmoContext::SetAxisLimit(float value) { impl_->SetAxisLimit(value); }
void osc::ui::gizmo::detail::GizmoContext::SetAxisMask(bool x, bool y, bool z) { impl_->SetAxisMask(x, y, z); }
void osc::ui::gizmo::detail::GizmoContext::SetPlaneLimit(float value) { impl_->SetPlaneLimit(value); }
std::optional<Transform> osc::ui::gizmo::detail::GizmoContext::Manipulate(
    const Matrix4x4& view,
    const Matrix4x4& projection,
    Operation operation,
    Mode mode,
    Matrix4x4& matrix,
    const GizmoOperationSnappingSteps* snap,
    const AABB* localBounds,
    const float* boundsSnap)
{
   return impl_->Manipulate(view, projection, operation, mode, matrix, snap, localBounds, boundsSnap);
}

// NOLINTEND
