#include "GuiRuler.hpp"

#include "oscar/Graphics/SceneCollision.hpp"
#include "oscar/Maths/PolarPerspectiveCamera.hpp"
#include "oscar/Maths/Rect.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <SDL_events.h>

#include <array>
#include <cstdio>
#include <optional>
#include <string>
#include <utility>

void osc::GuiRuler::draw(
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

    glm::vec2 mouseLoc = ImGui::GetMousePos();
    ImDrawList& dl = *ImGui::GetWindowDrawList();
    ImU32 circleMousedOverNothingColor = ImGui::ColorConvertFloat4ToU32({1.0f, 0.0f, 0.0f, 0.6f});
    ImU32 circleColor = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 0.8f});
    ImU32 lineColor = circleColor;
    ImU32 textBgColor = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 1.0f});
    ImU32 textColor = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
    float circleRadius = 5.0f;
    float lineThickness = 3.0f;
    glm::vec2 labelOffsetWhenNoLine = {10.0f, -10.0f};

    auto drawTooltipWithBg = [&dl, &textBgColor, &textColor](glm::vec2 const& pos, char const* text)
    {
        glm::vec2 sz = ImGui::CalcTextSize(text);
        float bgPad = 5.0f;
        float edgeRounding = bgPad - 2.0f;

        dl.AddRectFilled(pos - bgPad, pos + sz + bgPad, textBgColor, edgeRounding);
        dl.AddText(pos, textColor, text);
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
        glm::vec2 startScreenPos = sceneCamera.projectOntoScreenRect(m_StartWorldPos, renderRect);

        if (maybeMouseover)
        {
            // user is moused over something, so draw a line + circle between the two hitlocs
            glm::vec2 endScreenPos = mouseLoc;
            glm::vec2 lineScreenDir = glm::normalize(startScreenPos - endScreenPos);
            glm::vec2 offsetVec = 15.0f * glm::vec2{lineScreenDir.y, -lineScreenDir.x};
            glm::vec2 lineMidpoint = (startScreenPos + endScreenPos) / 2.0f;
            float lineWorldLen = glm::length(maybeMouseover->worldspaceLocation - m_StartWorldPos);

            dl.AddCircleFilled(startScreenPos, circleRadius, circleColor);
            dl.AddLine(startScreenPos, endScreenPos, lineColor, lineThickness);
            dl.AddCircleFilled(endScreenPos, circleRadius, circleColor);

            // label the line's length
            {
                std::array<char, 64> buf{};
                std::snprintf(buf.data(), buf.size(), "%.5f", lineWorldLen);
                drawTooltipWithBg(lineMidpoint + offsetVec, buf.data());
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