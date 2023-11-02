#include "GuiRuler.hpp"

#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Scene/SceneCollision.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <imgui.h>

#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

void osc::GuiRuler::onDraw(
    PolarPerspectiveCamera const& sceneCamera,
    Rect const& renderRect,
    std::optional<SceneCollision> maybeMouseover)
{
    if (m_State == State::Inactive)
    {
        return;
    }

    // users can exit measuring through these actions
    if (ImGui::IsKeyDown(ImGuiKey_Escape) || ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        stopMeasuring();
        return;
    }

    // users can "finish" the measurement through these actions
    if (m_State == State::WaitingForSecondPoint && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        stopMeasuring();
        return;
    }

    Vec2 mouseLoc = ImGui::GetMousePos();
    ImDrawList& dl = *ImGui::GetWindowDrawList();
    ImU32 circleMousedOverNothingColor = ImGui::ColorConvertFloat4ToU32({1.0f, 0.0f, 0.0f, 0.6f});
    ImU32 circleColor = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 0.8f});
    ImU32 lineColor = circleColor;
    ImU32 textBgColor = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 1.0f});
    ImU32 textColor = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
    float circleRadius = 5.0f;
    float lineThickness = 3.0f;

    auto drawTooltipWithBg = [&dl, &textBgColor, &textColor](Vec2 const& pos, CStringView tooltipText)
    {
        Vec2 sz = ImGui::CalcTextSize(tooltipText.c_str());
        float bgPad = 5.0f;
        float edgeRounding = bgPad - 2.0f;

        dl.AddRectFilled(pos - bgPad, pos + sz + bgPad, textBgColor, edgeRounding);
        dl.AddText(pos, textColor, tooltipText.c_str());
    };

    if (m_State == State::WaitingForFirstPoint)
    {
        if (!maybeMouseover)
        {
            // not mousing over anything
            dl.AddCircleFilled(mouseLoc, circleRadius, circleMousedOverNothingColor);
            return;
        }
        else
        {
            // mousing over something
            dl.AddCircleFilled(mouseLoc, circleRadius, circleColor);

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                m_State = State::WaitingForSecondPoint;
                m_StartWorldPos = maybeMouseover->worldspaceLocation;
            }
            return;
        }
    }
    else if (m_State == State::WaitingForSecondPoint)
    {
        Vec2 startScreenPos = sceneCamera.projectOntoScreenRect(m_StartWorldPos, renderRect);

        if (maybeMouseover)
        {
            // user is moused over something, so draw a line + circle between the two hitlocs
            Vec2 endScreenPos = mouseLoc;
            Vec2 lineScreenDir = Normalize(startScreenPos - endScreenPos);
            Vec2 offsetVec = 15.0f * Vec2{lineScreenDir.y, -lineScreenDir.x};
            Vec2 lineMidpoint = (startScreenPos + endScreenPos) / 2.0f;
            float lineWorldLen = Length(maybeMouseover->worldspaceLocation - m_StartWorldPos);

            dl.AddCircleFilled(startScreenPos, circleRadius, circleColor);
            dl.AddLine(startScreenPos, endScreenPos, lineColor, lineThickness);
            dl.AddCircleFilled(endScreenPos, circleRadius, circleColor);

            // label the line's length
            {
                std::string const lineLenLabel = [&lineWorldLen]()
                {
                    std::stringstream ss;
                    ss << std::setprecision(5) << lineWorldLen;
                    return std::move(ss).str();
                }();
                drawTooltipWithBg(lineMidpoint + offsetVec, lineLenLabel);
            }
        }
        else
        {
            dl.AddCircleFilled(startScreenPos, circleRadius, circleColor);
        }
    }
}

void osc::GuiRuler::startMeasuring()
{
    m_State = State::WaitingForFirstPoint;
}

void osc::GuiRuler::stopMeasuring()
{
    m_State = State::Inactive;
}

void osc::GuiRuler::toggleMeasuring()
{
    if (m_State == State::Inactive)
    {
        m_State = State::WaitingForFirstPoint;
    }
    else
    {
        m_State = State::Inactive;
    }
}

bool osc::GuiRuler::isMeasuring() const
{
    return m_State != State::Inactive;
}
