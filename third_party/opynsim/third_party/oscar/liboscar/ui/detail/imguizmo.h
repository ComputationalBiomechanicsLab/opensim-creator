#pragma once

// This file was originally copied from a third-party source:
//
// - https://github.com/CedricGuillemet/ImGuizmo
// - commit: 6d588209f99b1324a608783d1f52faa518886c29
// - Licensed under the MIT license
// - Copyright(c) 2021 Cedric Guillemet
//
// Subsequent modifications to this file (see VCS) are copyright (c) 2024 Adam Kewley.
//
// ORIGINAL LICENSE TEXT (6d588209f99b1324a608783d1f52faa518886c29):
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

#include <liboscar/maths/angle.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/uid.h>

#include <optional>
#include <utility>

struct ImDrawList;

namespace osc::ui::gizmo::detail
{
    enum class Operation {
        None           =  0,
        TranslateX     = (1u << 0),
        TranslateY     = (1u << 1),
        TranslateZ     = (1u << 2),
        RotateX        = (1u << 3),
        RotateY        = (1u << 4),
        RotateZ        = (1u << 5),
        RotateInScreen = (1u << 6),
        ScaleX         = (1u << 7),
        ScaleY         = (1u << 8),
        ScaleZ         = (1u << 9),
        Bounds         = (1u << 10),
        ScaleXU        = (1u << 11),
        ScaleYU        = (1u << 12),
        ScaleZU        = (1u << 13),

        Translate      = TranslateX | TranslateY | TranslateZ,
        Rotate         = RotateX | RotateY | RotateZ | RotateInScreen,
        Scale          = ScaleX | ScaleY | ScaleZ,
        ScaleU         = ScaleXU | ScaleYU | ScaleZU, // universal
        Universal      = Translate | Rotate | ScaleU
    };

    constexpr Operation operator|(Operation lhs, Operation rhs)
    {
        return static_cast<Operation>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    constexpr Operation operator<<(Operation lhs, int rhs)
    {
        return static_cast<Operation>(std::to_underlying(lhs) << rhs);
    }

    constexpr Operation operator>>(Operation lhs, int rhs)
    {
        return static_cast<Operation>(std::to_underlying(lhs) >> rhs);
    }

    constexpr bool operator&(Operation lhs, Operation rhs)
    {
        return static_cast<bool>(std::to_underlying(lhs) & std::to_underlying(rhs));
    }

    enum class Mode {
        Local,
        World
    };

    // Returns the padding distance between the gizmo and any textual annotations
    // on it in device-independent pixels.
    constexpr float AnnotationOffset() { return 15.0f; }

    void CreateContext();
    void DestroyContext();

    // call inside your own window and before Manipulate() in order to draw gizmo to that window.
    // Or pass a specific ImDrawList to draw to (e.g. ImGui::GetForegroundDrawList()).
    void SetDrawlist(ImDrawList* drawlist = nullptr);

    // call BeginFrame right after ImGui_XXXX_NewFrame();
    void BeginFrame();

    // return true if mouse cursor is over any gizmo control (axis, plan or screen component)
    bool IsOver();

    // return true if the cursor is over the operation's gizmo
    bool IsOver(Operation op);

    // return true if mouse IsOver or if the gizmo is in moving state
    bool IsUsing();

    // return true if any gizmo is in moving state
    bool IsUsingAny();

    // enable/disable the gizmo. Stay in the state until next call to Enable.
    // gizmo is rendered with gray half transparent color when disabled
    void Enable(bool enable);

    // Set the viewport rectangle in the ui coordinate system (device-independent pixels)
    // where the gizmo shall be drawn.
    void SetRect(const Rect& ui_rect);

    // default is false
    void SetOrthographic(bool isOrthographic);

    // Push/Pop IDs from the current gizmo context's local ID stack
    void          PushID(UID);
    void          PopID();                                                        // pop from the ID stack.

    void SetGizmoSizeClipSpace(float value);

    // Configure the limit where axis are hidden
    void SetAxisLimit(float value);
    // Set an axis mask to permanently hide a given axis (true -> hidden, false -> shown)
    void SetAxisMask(bool x, bool y, bool z);
    // Configure the limit where planes are hidden
    void SetPlaneLimit(float value);

    // Represents the step size that the gizmo should stick to when the user is
    // using a gizmo operation.
    struct OperationSnappingSteps final {
        std::optional<Vector3> scale;
        std::optional<Radians> rotation;
        std::optional<Vector3> position;
    };

    // call it when you want a gizmo
    // Needs view and projection matrices.
    // matrix parameter is the source matrix (where will be gizmo be drawn) and might be transformed by the function.
    // translation is applied in world space
    std::optional<Transform> Manipulate(
        const Matrix4x4& view,
        const Matrix4x4& projection,
        Operation operation,
        Mode mode,
        Matrix4x4& matrix,
        std::optional<OperationSnappingSteps> snap = std::nullopt,
        const float* localBounds = nullptr,
        const float* boundsSnap = nullptr
    );
}
