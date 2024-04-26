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
#include <cmath>
#include <iterator>
#include <ranges>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    struct AxesMetrics final {
        float fontSize = ui::GetFontSize();
        float linelen = 2.0f * fontSize;
        float circleRadius = 0.6f * fontSize;
        float maxEdgeLength = 2.0f * (linelen + std::sqrt(2.0f * circleRadius * circleRadius));
        Vec2 dimensions = {maxEdgeLength, maxEdgeLength};
    };
}

Vec2 osc::CameraViewAxes::dimensions() const
{
    return AxesMetrics{}.dimensions;
}

bool osc::CameraViewAxes::draw(PolarPerspectiveCamera& camera)
{
    // calculate widget metrics
    const auto metrics = AxesMetrics{};

    // calculate widget screen-space metrics
    const Vec2 topLeft = ui::GetCursorScreenPos();
    const Rect bounds = {topLeft, topLeft + metrics.dimensions};
    const Vec2 origin = centroid_of(bounds);

    // figure out rendering order (back-to-front)
    const Mat4 viewMtx = camera.view_matrix();
    auto order = std::to_array<Vec4::size_type>({0, 1, 2});
    rgs::sort(order, [&viewMtx](auto a, auto b)
    {
        const float aDepth = (viewMtx * Vec4{}.with_element(a, 1.0f)).z;
        const float bDepth = (viewMtx * Vec4{}.with_element(b, 1.0f)).z;
        return aDepth < bDepth;
    });

    // draw each edge back-to-front
    bool edited = false;
    ImDrawList& drawlist = *ui::GetWindowDrawList();
    for (auto i : order) {
        // calc direction vector in screen space
        Vec2 view = Vec2{viewMtx * Vec4{}.with_element(i, 1.0f)};
        view.y = -view.y;  // y goes down in screen-space

        Color baseColor = {0.15f, 0.15f, 0.15f, 1.0f};
        baseColor[i] = 0.7f;

        // draw line from origin to end with a labelled (clickable) circle ending
        {
            const Vec2 end = origin + metrics.linelen*view;
            const Circle circ = {.origin = end, .radius = metrics.circleRadius};
            const Rect circleBounds = bounding_rect_of(circ);

            const auto labels = std::to_array<CStringView>({ "X", "Y", "Z" });
            const auto id = ui::GetID(labels[i]);
            ui::SetCursorScreenPos(circleBounds.p1);
            ui::ItemSize(circleBounds);
            if (ui::ItemAdd(circleBounds, id)) {
                const Vec2 labelSize = ui::CalcTextSize(labels[i]);

                const bool hovered = ui::ItemHoverable(circleBounds, id, ui::GetItemFlags());
                const ImU32 color = ui::ToImU32(hovered ? Color::white() : baseColor);
                const ImU32 textColor = ui::ToImU32(hovered ? Color::black() : Color::white());

                drawlist.AddLine(origin, end, color, 3.0f);
                drawlist.AddCircleFilled(circ.origin, circ.radius, color);
                drawlist.AddText(end - 0.5f*labelSize, textColor, labels[i].c_str());

                if (hovered && ui::IsMouseClicked(ImGuiMouseButton_Left, id)) {
                    FocusAlongAxis(camera, i);
                    edited = true;
                }
            }
        }

        // negative axes: draw a faded (clickable) circle ending - no line
        {
            const Vec2 end = origin - metrics.linelen*view;
            const Circle circ = {.origin = end, .radius = metrics.circleRadius};
            const Rect circleBounds = bounding_rect_of(circ);

            const auto labels = std::to_array<CStringView>({ "-X", "-Y", "-Z" });
            const auto id = ui::GetID(labels[i]);
            ui::SetCursorScreenPos(circleBounds.p1);
            ui::ItemSize(circleBounds);
            if (ui::ItemAdd(circleBounds, id)) {
                const bool hovered = ui::ItemHoverable(circleBounds, id, ui::GetItemFlags());
                const ImU32 color = ui::ToImU32(hovered ? Color::white() : baseColor.with_alpha(0.3f));

                drawlist.AddCircleFilled(circ.origin, circ.radius, color);

                if (hovered && ui::IsMouseClicked(ImGuiMouseButton_Left, id)) {
                    FocusAlongAxis(camera, i, true);
                    edited = true;
                }
            }
        }
    }

    return edited;
}
