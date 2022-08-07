#include "ExperimentsScreen.hpp"

#include "src/Platform/App.hpp"
#include "src/Screens/MainUIScreen.hpp"
#include "src/Screens/MeshHittestScreen.hpp"
#include "src/Screens/MeshHittestWithBVHScreen.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>

#include <string>
#include <utility>
#include <vector>

template<typename Screen>
static void transition()
{
    osc::App::upd().requestTransition<Screen>();
}

using transition_fn = void(*)(void);
struct Entry final { std::string name; transition_fn f;  };

// experiments screen impl
class osc::ExperimentsScreen::Impl final {
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
            return;  // ImGui handled it
        }
    }

    void onDraw()
    {
        App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
        osc::ImGuiNewFrame();

        glm::vec2 dims = App::get().dims();
        glm::vec2 menuDims = {700.0f, 500.0f};

        // center the menu
        {
            glm::vec2 menu_pos = (dims - menuDims) / 2.0f;
            ImGui::SetNextWindowPos(menu_pos);
            ImGui::SetNextWindowSize(ImVec2(menuDims.x, -1));
            ImGui::SetNextWindowSizeConstraints(menuDims, menuDims);
        }

        ImGui::Begin("select experiment");

        ImGui::Dummy({0.0f, 0.5f * ImGui::GetTextLineHeight()});
        if (ImGui::Button(ICON_FA_HOME " back to main UI"))
        {
            App::upd().requestTransition<osc::MainUIScreen>();
        }
        ImGui::Dummy({0.0f, 0.5f * ImGui::GetTextLineHeight()});

        ImGui::Separator();

        for (auto const& e : m_Entries)
        {
            if (ImGui::Button(e.name.c_str()))
            {
                e.f();
            }
        }

        ImGui::End();

        osc::ImGuiRender();
    }

private:
    std::vector<Entry> m_Entries =
    {
        { "Hit testing ray-triangle intersections in a mesh", transition<MeshHittestScreen> },
        { "Hit testing ray-triangle, but with BVH acceleration", transition<MeshHittestWithBVHScreen> },
    };
};

// public API (PIMPL)

osc::ExperimentsScreen::ExperimentsScreen() :
    m_Impl{new Impl{}}
{
}

osc::ExperimentsScreen::ExperimentsScreen(ExperimentsScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ExperimentsScreen& osc::ExperimentsScreen::operator=(ExperimentsScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ExperimentsScreen::~ExperimentsScreen() noexcept
{
    delete m_Impl;
}

void osc::ExperimentsScreen::onMount()
{
    m_Impl->onMount();
}

void osc::ExperimentsScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::ExperimentsScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::ExperimentsScreen::onDraw()
{
    m_Impl->onDraw();
}
