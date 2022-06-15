#include "MathExperimentsScreen.hpp"

#include "src/Maths/Geometry.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"

#include <imgui.h>
#include <glm/vec2.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <utility>

class osc::MathExperimentsScreen::Impl final {
public:
    void onMount()
    {
        osc::ImGuiInit();
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
    }

    void onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_QUIT)
        {
            App::upd().requestQuit();
            return;
        }
        else if (osc::ImGuiOnEvent(e))
        {
            return;  // ImGui handled this particular event
        }
    }

    void onTick()
    {
    }

    void onDraw()
    {
        osc::ImGuiNewFrame();  // tell ImGui you're about to start drawing a new frame

        App::upd().clearScreen({1.0f, 1.0f, 1.0f, 1.0f});  // set app window bg color

        glm::vec2 screenCenter = ImGui::GetMainViewport()->GetCenter();
        glm::vec2 mousePos = ImGui::GetIO().MousePos;
        glm::vec2 mainVec = mousePos - screenCenter;

        ImDrawList* dl = ImGui::GetForegroundDrawList();

        // draw vector
        dl->AddLine(screenCenter, mousePos, 0xff000000, 1.0f);

        // draw x component
        {
            glm::vec2 xBegin = screenCenter;
            glm::vec2 xEnd = {mousePos.x, screenCenter.y};
            ImVec2 xMid = ImVec2{xBegin.x + (xEnd.x - xBegin.x)/2.0f, xBegin.y};

            char buf[256];
            std::snprintf(buf, sizeof(buf), "%.3f", xEnd.x - xBegin.x);

            dl->AddLine(xBegin, xEnd, 0xffaaaaaa, 1.0f);
            dl->AddText(xMid, 0xff000000, buf);
        }

        // draw y component
        {
            ImVec2 yBegin = screenCenter;
            ImVec2 yEnd = {screenCenter.x, mousePos.y};
            ImVec2 yMid = ImVec2{yBegin.x, yBegin.y + (yEnd.y - yBegin.y)/2.0f};

            char buf[256];
            std::snprintf(buf, sizeof(buf), "%.3f", yEnd.y - yBegin.y);

            dl->AddLine(yBegin, yEnd, 0xffaaaaaa, 1.0f);
            dl->AddText(yMid, 0xff000000, buf);
        }

        // draw other vector
        auto proj = [](glm::vec2 const& a, glm::vec2 const& b)
        {
            return glm::dot(a, b)/glm::dot(b, b) * b;
        };

        {
            glm::vec2 otherVec = {0.0, -50.0f};
            dl->AddLine(screenCenter, screenCenter + otherVec, 0xff00ff00, 2.0f);

            glm::vec2 projvec = proj(otherVec, mainVec);
            dl->AddLine(screenCenter, glm::vec2{screenCenter} + projvec, 0xffff0000, 2.0f);
        }

        ImGui::Begin("cookiecutter panel");
        ImGui::Text("screen center = %.2f, %.2f", screenCenter.x, screenCenter.y);
        ImGui::Text("mainvec = %.2f, %.2f", mainVec.x, mainVec.y);
        glm::vec2 relVec = ToMat4(boxTransform) * glm::vec4{mousePos, 0.0f, 1.0f};
        ImGui::Text("relvec (mtx) = %.2f, %.2f", relVec.x, relVec.y);
        glm::vec2 relVecF = TransformPoint(boxTransform, glm::vec3{mousePos, 0.0f});
        ImGui::Text("relvec (func) = %.2f, %.2f", relVecF.x, relVecF.y);

        osc::ImGuiRender();  // tell ImGui to render any ImGui widgets since calling ImGuiNewFrame();
    }

private:
    Transform boxTransform = Transform{{75.0f, 75.0f, 0.0f}};
};


// public API (PIMPL)

osc::MathExperimentsScreen::MathExperimentsScreen() :
    m_Impl{new Impl{}}
{
}

osc::MathExperimentsScreen::MathExperimentsScreen(MathExperimentsScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::MathExperimentsScreen& osc::MathExperimentsScreen::operator=(MathExperimentsScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::MathExperimentsScreen::~MathExperimentsScreen() noexcept
{
    delete m_Impl;
}

void osc::MathExperimentsScreen::onMount()
{
    m_Impl->onMount();
}

void osc::MathExperimentsScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::MathExperimentsScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::MathExperimentsScreen::onTick()
{
    m_Impl->onTick();
}

void osc::MathExperimentsScreen::onDraw()
{
    m_Impl->onDraw();
}
