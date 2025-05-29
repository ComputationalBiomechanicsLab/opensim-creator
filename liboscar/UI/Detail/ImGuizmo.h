#pragma once

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
//
// LIBOSCAR CHANGES:
//
// This header was originally copied from a third-party source:
//
// - https://github.com/CedricGuillemet/ImGuizmo
// - commit: 6d588209f99b1324a608783d1f52faa518886c29
// - Licensed under the MIT license
// - Copyright(c) 2021 Cedric Guillemet
//
// Subsequent modifications to this file are licensed under liboscar's
// license (see LICENSE/NOTICE.txt) and have the following copyright:
//
// - Copyright (c) 2024 Adam Kewley
//
// Individual lines are not tagged with copyright/license notices, because
// that would be insanely impractical. The project's vcs (i.e. git) can be
// used to track each change.

#include <liboscar/Utils/UID.h>

struct ImDrawList;

namespace ImGuizmo
{
    enum OPERATION {
        NONE             =  0,
        TRANSLATE_X      = (1u << 0),
        TRANSLATE_Y      = (1u << 1),
        TRANSLATE_Z      = (1u << 2),
        ROTATE_X         = (1u << 3),
        ROTATE_Y         = (1u << 4),
        ROTATE_Z         = (1u << 5),
        ROTATE_SCREEN    = (1u << 6),
        SCALE_X          = (1u << 7),
        SCALE_Y          = (1u << 8),
        SCALE_Z          = (1u << 9),
        BOUNDS           = (1u << 10),
        SCALE_XU         = (1u << 11),
        SCALE_YU         = (1u << 12),
        SCALE_ZU         = (1u << 13),

        TRANSLATE = TRANSLATE_X | TRANSLATE_Y | TRANSLATE_Z,
        ROTATE = ROTATE_X | ROTATE_Y | ROTATE_Z | ROTATE_SCREEN,
        SCALE = SCALE_X | SCALE_Y | SCALE_Z,
        SCALEU = SCALE_XU | SCALE_YU | SCALE_ZU, // universal
        UNIVERSAL = TRANSLATE | ROTATE | SCALEU
    };

    inline constexpr OPERATION operator|(OPERATION lhs, OPERATION rhs)
    {
        return static_cast<OPERATION>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    enum MODE {
        LOCAL,
        WORLD
    };

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

    inline constexpr float AnnotationOffset() { return 15.0f; }

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
    bool IsOver(OPERATION op);

    // return true if mouse IsOver or if the gizmo is in moving state
    bool IsUsing();

    // return true if any gizmo is in moving state
    bool IsUsingAny();

    // enable/disable the gizmo. Stay in the state until next call to Enable.
    // gizmo is rendered with gray half transparent color when disabled
    void Enable(bool enable);

    void SetRect(float x, float y, float width, float height);

    // default is false
    void SetOrthographic(bool isOrthographic);

    // call it when you want a gizmo
    // Needs view and projection matrices.
    // matrix parameter is the source matrix (where will be gizmo be drawn) and might be transformed by the function. Return deltaMatrix is optional
    // translation is applied in world space
    bool Manipulate(
        const float* view,
        const float* projection,
        OPERATION operation,
        MODE mode,
        float* matrix,
        float* deltaMatrix = NULL,
        const float* snap = NULL,
        const float* localBounds = NULL,
        const float* boundsSnap = NULL
    );

    // Push/Pop IDs from ImGuizmo's local ID stack
    void          PushID(osc::UID);
    void          PopID();                                                        // pop from the ID stack.

    void SetGizmoSizeClipSpace(float value);

    // Configure the limit where axis are hidden
    void SetAxisLimit(float value);
    // Set an axis mask to permanently hide a given axis (true -> hidden, false -> shown)
    void SetAxisMask(bool x, bool y, bool z);
    // Configure the limit where planes are hiden
    void SetPlaneLimit(float value);
}
