#include "CameraViewAxes.h"

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/Utils/CStringView.h>

#include <imgui.h>

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

Rect osc::CameraViewAxes::draw(PolarPerspectiveCamera& camera)
{
    Mat4 const viewMtx = camera.getViewMtx();

    auto const metrics = AxesMetrics{};
    ImU32 const whiteColorU32 = ToImU32(Color::white());

    Vec2 const topLeft = ImGui::GetCursorScreenPos();
    Vec2 const bottomRight = topLeft + metrics.dimensions;
    Rect const bounds = {topLeft, bottomRight};
    Vec2 const origin = Midpoint(bounds);

    auto const labels = std::to_array<CStringView>({ "X", "Y", "Z" });

    ImDrawList& drawlist = *ImGui::GetWindowDrawList();
    for (size_t i = 0; i < std::size(labels); ++i)
    {
        Vec4 world = {0.0f, 0.0f, 0.0f, 0.0f};
        world[static_cast<Vec4::size_type>(i)] = 1.0f;

        Vec2 view = Vec2{viewMtx * world};
        view.y = -view.y;  // y goes down in screen-space

        Vec2 const p1 = origin;
        Vec2 const p2 = origin + metrics.linelen*view;

        Color color = {0.15f, 0.15f, 0.15f, 1.0f};
        color[i] = 0.7f;
        ImU32 const colorU32 = ToImU32(color);

        Vec2 const ts = ImGui::CalcTextSize(labels[i].c_str());

        drawlist.AddLine(p1, p2, colorU32, 3.0f);
        drawlist.AddCircleFilled(p2, metrics.circleRadius, colorU32);
        drawlist.AddText(p2 - 0.5f*ts, whiteColorU32, labels[i].c_str());

        // also, add a faded line for symmetry
        {
            color.a *= 0.15f;
            ImU32 const colorFadedU32 = ToImU32(color);
            Vec2 const p2rev = origin - metrics.linelen*view;
            drawlist.AddLine(p1, p2rev, colorFadedU32, 3.0f);
        }
    }

    return bounds;
}
