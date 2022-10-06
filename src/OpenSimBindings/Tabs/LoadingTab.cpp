#include "LoadingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Rect.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/Tabs/ModelEditorTab.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Tabs/TabHost.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <SDL_events.h>

#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <utility>

class osc::LoadingTab::Impl final {
public:

    Impl(MainUIStateAPI* parent, std::filesystem::path path) :
        m_Parent{std::move(parent)},
        m_OsimPath{std::move(path)},
        m_LoadingResult{std::async(std::launch::async, osc::LoadOsimIntoUndoableModel, m_OsimPath)}
    {
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    void onMount()
    {
    }

    void onUnmount()
    {
    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {
        float dt = osc::App::get().getDeltaSinceLastFrame().count();

        // tick the progress bar up a little bit
        m_LoadingProgress += (dt * (1.0f - m_LoadingProgress))/2.0f;

        // if there's an error, then the result came through (it's an error)
        // and this screen should just continuously show the error until the
        // user decides to transition back
        if (!m_LoadingErrorMsg.empty())
        {
            return;
        }

        // otherwise, poll for the result and catch any exceptions that bubble
        // up from the background thread
        std::unique_ptr<UndoableModelStatePair> result = nullptr;
        try
        {
            if (m_LoadingResult.wait_for(std::chrono::seconds{0}) == std::future_status::ready)
            {
                result = m_LoadingResult.get();
            }
        }
        catch (std::exception const& ex)
        {
            m_LoadingErrorMsg = ex.what();
            return;
        }
        catch (...)
        {
            m_LoadingErrorMsg = "an unknown exception (does not inherit from std::exception) occurred when loading the file";
            return;
        }

        // if there was a result (a newly-loaded model), handle it
        if (result)
        {
            // add newly-loaded model to the "Recent Files" list
            App::upd().addRecentFile(m_OsimPath);

            // there is an existing editor state
            //
            // recycle it so that users can keep their running sims, local edits, etc.
            UID tabID = m_Parent->addTab<ModelEditorTab>(m_Parent, std::move(result));
            m_Parent->selectTab(tabID);
            m_Parent->closeTab(m_ID);
        }
    }

    void onDrawMainMenu()
    {
    }

    void onDraw()
    {
        constexpr glm::vec2 menu_dims = {512.0f, 512.0f};

        Rect tabRect = osc::GetMainViewportWorkspaceScreenRect();
        glm::vec2 window_dims = osc::Dimensions(tabRect);

        // center the menu
        {
            glm::vec2 menu_pos = (window_dims - menu_dims) / 2.0f;
            ImGui::SetNextWindowPos(menu_pos);
            ImGui::SetNextWindowSize(ImVec2(menu_dims.x, -1));
        }

        if (m_LoadingErrorMsg.empty())
        {
            if (ImGui::Begin("Loading Message", nullptr, ImGuiWindowFlags_NoTitleBar))
            {
                ImGui::Text("loading: %s", m_OsimPath.string().c_str());
                ImGui::ProgressBar(m_LoadingProgress);
            }
            ImGui::End();
        }
        else
        {
            if (ImGui::Begin("Error Message", nullptr, ImGuiWindowFlags_NoTitleBar))
            {
                ImGui::TextWrapped("An error occurred while loading the file:");
                ImGui::Dummy({0.0f, 5.0f});
                ImGui::TextWrapped("%s", m_LoadingErrorMsg.c_str());
                ImGui::Dummy({0.0f, 5.0f});

                if (ImGui::Button("try again"))
                {
                    UID tabID = m_Parent->addTab<LoadingTab>(m_Parent, m_OsimPath);
                    m_Parent->selectTab(tabID);
                    m_Parent->closeTab(m_ID);
                }
            }
            ImGui::End();
        }
    }


private:
    // ID of the tab
    UID m_ID;

    // display name of the tab
    std::string m_Name = "LoadingTab";

    // main UI shared state API
    MainUIStateAPI* m_Parent;

    // filesystem path to the osim being loaded
    std::filesystem::path m_OsimPath;

    // future that lets the UI thread poll the loading thread for
    // the loaded model
    std::future<std::unique_ptr<osc::UndoableModelStatePair>> m_LoadingResult;

    // if not empty, any error encountered by the loading thread
    std::string m_LoadingErrorMsg;

    // a fake progress indicator that never quite reaches 100 %
    //
    // this might seem evil, but its main purpose is to ensure the
    // user that *something* is happening - even if that "something"
    // is "the background thread is deadlocked" ;)
    float m_LoadingProgress = 0.0f;
};


// public API

osc::LoadingTab::LoadingTab(MainUIStateAPI* parent, std::filesystem::path path) :
    m_Impl{new Impl{std::move(parent), std::move(path)}}
{
}

osc::LoadingTab::LoadingTab(LoadingTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::LoadingTab& osc::LoadingTab::operator=(LoadingTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::LoadingTab::~LoadingTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::LoadingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LoadingTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::LoadingTab::implParent() const
{
    return m_Impl->parent();
}

void osc::LoadingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LoadingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LoadingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LoadingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LoadingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LoadingTab::implOnDraw()
{
    m_Impl->onDraw();
}
