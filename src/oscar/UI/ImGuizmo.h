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
// OPENSIM CREATOR CHANGES:
//
// This header was originally copied from a third-party source:
//
// - https://github.com/CedricGuillemet/ImGuizmo
// - commit: 6d588209f99b1324a608783d1f52faa518886c29
// - Licensed under the MIT license
// - Copyright(c) 2021 Cedric Guillemet
//
// Subsequent modifications to this file are licensed under OpenSim
// Creator's license (see LICENSE/NOTICE.txt) and have the following
// copyright:
//
// - Copyright (c) 2024 Adam Kewley
//
// Individual lines are not tagged with copyright/license notices, because
// that would be insanely impractical. The project's vcs (i.e. git) can be
// used to track each change.

struct ImGuiWindow;
struct ImDrawList;
struct ImGuiContext;
struct ImVec2;
struct ImVec4;
using ImGuiID = unsigned int;
using ImU32 = unsigned int;

namespace ImGuizmo
{
   void CreateContext();
   void DestroyContext();

   // call inside your own window and before Manipulate() in order to draw gizmo to that window.
   // Or pass a specific ImDrawList to draw to (e.g. ImGui::GetForegroundDrawList()).
   void SetDrawlist(ImDrawList* drawlist = nullptr);

   // call BeginFrame right after ImGui_XXXX_NewFrame();
   void BeginFrame();

   // this is necessary because when imguizmo is compiled into a dll, and imgui into another
   // globals are not shared between them.
   // More details at https://stackoverflow.com/questions/19373061/what-happens-to-global-and-static-variables-in-a-shared-library-when-it-is-dynam
   // expose method to set imgui context
   void SetImGuiContext(ImGuiContext* ctx);

   // return true if mouse cursor is over any gizmo control (axis, plan or screen component)
   bool IsOver();

   // return true if mouse IsOver or if the gizmo is in moving state
   bool IsUsing();

   // return true if the view gizmo is in moving state
   bool IsUsingViewManipulate();

   // return true if any gizmo is in moving state
   bool IsUsingAny();

   // enable/disable the gizmo. Stay in the state until next call to Enable.
   // gizmo is rendered with gray half transparent color when disabled
   void Enable(bool enable);

   // helper functions for manualy editing translation/rotation/scale with an input float
   // translation, rotation and scale float points to 3 floats each
   // Angles are in degrees (more suitable for human editing)
   // example:
   // float matrixTranslation[3], matrixRotation[3], matrixScale[3];
   // ImGuizmo::DecomposeMatrixToComponents(gizmoMatrix.m16, matrixTranslation, matrixRotation, matrixScale);
   // ImGui::InputFloat3("Tr", matrixTranslation, 3);
   // ImGui::InputFloat3("Rt", matrixRotation, 3);
   // ImGui::InputFloat3("Sc", matrixScale, 3);
   // ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, gizmoMatrix.m16);
   //
   // These functions have some numerical stability issues for now. Use with caution.
   void DecomposeMatrixToComponents(const float* matrix, float* translation, float* rotation, float* scale);
   void RecomposeMatrixFromComponents(const float* translation, const float* rotation, const float* scale, float* matrix);

   void SetRect(float x, float y, float width, float height);
   // default is false
   void SetOrthographic(bool isOrthographic);

   // Render a cube with face color corresponding to face normal. Usefull for debug/tests
   void DrawCubes(const float* view, const float* projection, const float* matrices, int matrixCount);
   void DrawGrid(const float* view, const float* projection, const float* matrix, const float gridSize);

   // call it when you want a gizmo
   // Needs view and projection matrices.
   // matrix parameter is the source matrix (where will be gizmo be drawn) and might be transformed by the function. Return deltaMatrix is optional
   // translation is applied in world space
   enum OPERATION
   {
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

   inline OPERATION operator|(OPERATION lhs, OPERATION rhs)
   {
     return static_cast<OPERATION>(static_cast<int>(lhs) | static_cast<int>(rhs));
   }

   enum MODE
   {
      LOCAL,
      WORLD
   };

   bool Manipulate(const float* view, const float* projection, OPERATION operation, MODE mode, float* matrix, float* deltaMatrix = NULL, const float* snap = NULL, const float* localBounds = NULL, const float* boundsSnap = NULL);
   //
   // Please note that this cubeview is patented by Autodesk : https://patents.google.com/patent/US7782319B2/en
   // It seems to be a defensive patent in the US. I don't think it will bring troubles using it as
   // other software are using the same mechanics. But just in case, you are now warned!
   //
   void ViewManipulate(float* view, float length, ImVec2 position, ImVec2 size, ImU32 backgroundColor);

   // use this version if you did not call Manipulate before and you are just using ViewManipulate
   void ViewManipulate(float* view, const float* projection, OPERATION operation, MODE mode, float* matrix, float length, ImVec2 position, ImVec2 size, ImU32 backgroundColor);

   void SetAlternativeWindow(ImGuiWindow* window);

   [[deprecated("Use PushID/PopID instead.")]]
   void SetID(int id);

    // ID stack/scopes
    // Read the FAQ (docs/FAQ.md or http://dearimgui.org/faq) for more details about how ID are handled in dear imgui.
    // - Those questions are answered and impacted by understanding of the ID stack system:
    //   - "Q: Why is my widget not reacting when I click on it?"
    //   - "Q: How can I have widgets with an empty label?"
    //   - "Q: How can I have multiple widgets with the same label?"
    // - Short version: ID are hashes of the entire ID stack. If you are creating widgets in a loop you most likely
    //   want to push a unique identifier (e.g. object pointer, loop index) to uniquely differentiate them.
    // - You can also use the "Label##foobar" syntax within widget label to distinguish them from each others.
    // - In this header file we use the "label"/"name" terminology to denote a string that will be displayed + used as an ID,
    //   whereas "str_id" denote a string that is only used as an ID and not normally displayed.
    void          PushID(const char* str_id);                                     // push string into the ID stack (will hash string).
    void          PushID(const char* str_id_begin, const char* str_id_end);       // push string into the ID stack (will hash string).
    void          PushID(const void* ptr_id);                                     // push pointer into the ID stack (will hash pointer).
    void          PushID(int int_id);                                             // push integer into the ID stack (will hash integer).
    void          PopID();                                                        // pop from the ID stack.
    ImGuiID       GetID(const char* str_id);                                      // calculate unique ID (hash of whole ID stack + given parameter). e.g. if you want to query into ImGuiStorage yourself
    ImGuiID       GetID(const char* str_id_begin, const char* str_id_end);
    ImGuiID       GetID(const void* ptr_id);

   // return true if the cursor is over the operation's gizmo
   bool IsOver(OPERATION op);
   void SetGizmoSizeClipSpace(float value);

   // Allow axis to flip
   // When true (default), the guizmo axis flip for better visibility
   // When false, they always stay along the positive world/local axis
   void AllowAxisFlip(bool value);

   // Configure the limit where axis are hidden
   void SetAxisLimit(float value);
   // Set an axis mask to permanently hide a given axis (true -> hidden, false -> shown)
   void SetAxisMask(bool x, bool y, bool z);
   // Configure the limit where planes are hiden
   void SetPlaneLimit(float value);
   // from a x,y,z point in space and using Manipulation view/projection matrix, check if mouse is in pixel radius distance of that projected point
   bool IsOver(float* position, float pixelRadius);

   enum COLOR
   {
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
}
