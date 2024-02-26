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

Rect osc::CameraViewAxes::draw(PolarPerspectiveCamera& camera)
{
    // calculate widget metrics
    auto const metrics = AxesMetrics{};

    // calculate widget screen-space metrics
    Vec2 const topLeft = ImGui::GetCursorScreenPos();
    Rect const bounds = {topLeft, topLeft + metrics.dimensions};
    Vec2 const origin = Midpoint(bounds);

    Mat4 const viewMtx = camera.getViewMtx();

    auto const labels = std::to_array<CStringView>({ "X", "Y", "Z" });
    auto order = std::to_array<Vec4::size_type>({0, 1, 2});
    std::sort(order.begin(), order.end(), [&viewMtx](auto a, auto b)
    {
        Vec4 av{};
        av[a] = 1.0f;
        Vec4 bv{};
        bv[b] = 1.0f;
        return (viewMtx*av).z < (viewMtx*bv).z;
    });

    ImDrawList& drawlist = *ImGui::GetWindowDrawList();
    for (auto i : order) {
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
        drawlist.AddText(p2 - 0.5f*ts, ToImU32(Color::white()), labels[i].c_str());

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
