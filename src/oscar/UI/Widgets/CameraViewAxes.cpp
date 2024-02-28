#include "CameraViewAxes.h"

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Circle.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/oscimgui_internal.h>
#include <oscar/Utils/CStringView.h>

#include <algorithm>
#include <array>
#include <iterator>

using namespace osc;

namespace
{
    struct AxesMetrics final {
        float fontSize = ImGui::GetFontSize();
        float linelen = 2.0f * fontSize;
        float circleRadius = 0.6f * fontSize;
        float edgeLen = 2.0f * (linelen + circleRadius);
        Vec2 dimensions = {edgeLen, edgeLen};
    };
}

Vec2 osc::CameraViewAxes::dimensions() const
{
    return AxesMetrics{}.dimensions;
}

bool osc::CameraViewAxes::draw(PolarPerspectiveCamera& camera)
{
    // calculate widget metrics
    auto const metrics = AxesMetrics{};

    // calculate widget screen-space metrics
    Vec2 const topLeft = ImGui::GetCursorScreenPos();
    Rect const bounds = {topLeft, topLeft + metrics.dimensions};
    Vec2 const origin = centroid(bounds);

    // figure out rendering order (back-to-front)
    Mat4 const viewMtx = camera.view_matrix();
    auto order = std::to_array<Vec4::size_type>({0, 1, 2});
    std::sort(order.begin(), order.end(), [&viewMtx](auto a, auto b)
    {
        float const aDepth = (viewMtx * Vec4{}.with_element(a, 1.0f)).z;
        float const bDepth = (viewMtx * Vec4{}.with_element(b, 1.0f)).z;
        return aDepth < bDepth;
    });

    // draw each edge back-to-front
    bool edited = false;
    ImDrawList& drawlist = *ImGui::GetWindowDrawList();
    for (auto i : order) {
        // calc direction vector in screen space
        Vec2 view = Vec2{viewMtx * Vec4{}.with_element(i, 1.0f)};
        view.y = -view.y;  // y goes down in screen-space

        Color baseColor = {0.15f, 0.15f, 0.15f, 1.0f};
        baseColor[i] = 0.7f;

        // draw line from origin to end with a labelled (clickable) circle ending
        {
            Vec2 const end = origin + metrics.linelen*view;
            Circle const circ = {.origin = end, .radius = metrics.circleRadius};
            Rect const circleBounds = BoundingRectOf(circ);
            ImRect const imCircleBounds = {circleBounds.p1, circleBounds.p2};

            auto const labels = std::to_array<CStringView>({ "X", "Y", "Z" });
            auto const id = ImGui::GetID(labels[i].c_str());
            ImGui::ItemSize(imCircleBounds);
            if (ImGui::ItemAdd(imCircleBounds, id)) {
                Vec2 const labelSize = ImGui::CalcTextSize(labels[i].c_str());

                bool const hovered = ImGui::ItemHoverable(imCircleBounds, id, ImGui::GetItemFlags());
                ImU32 const color = ToImU32(hovered ? Color::white() : baseColor);
                ImU32 const textColor = ToImU32(hovered ? Color::black() : Color::white());

                drawlist.AddLine(origin, end, color, 3.0f);
                drawlist.AddCircleFilled(circ.origin, circ.radius, color);
                drawlist.AddText(end - 0.5f*labelSize, textColor, labels[i].c_str());

                if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left, id)) {
                    FocusAlongAxis(camera, i);
                    edited = true;
                }
            }
        }

        // negative axes: draw a faded (clickable) circle ending - no line
        {
            Vec2 const end = origin - metrics.linelen*view;
            Circle const circ = {.origin = end, .radius = metrics.circleRadius};
            Rect const circleBounds = BoundingRectOf(circ);
            ImRect const imCircleBounds = {circleBounds.p1, circleBounds.p2};

            auto const labels = std::to_array<CStringView>({ "-X", "-Y", "-Z" });
            auto const id = ImGui::GetID(labels[i].c_str());
            ImGui::ItemSize(imCircleBounds);
            if (ImGui::ItemAdd(imCircleBounds, id)) {
                bool const hovered = ImGui::ItemHoverable(imCircleBounds, id, ImGui::GetItemFlags());
                ImU32 const color = ToImU32(hovered ? Color::white() : baseColor.withAlpha(0.3f));

                drawlist.AddCircleFilled(circ.origin, circ.radius, color);

                if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left, id)) {
                    FocusAlongAxis(camera, i, true);
                    edited = true;
                }
            }
        }
    }

    return edited;
}
