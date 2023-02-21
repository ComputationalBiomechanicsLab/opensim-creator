#include "LoadingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/Tabs/ModelEditorTab.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
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

    Impl(
        std::weak_ptr<MainUIStateAPI> parent_,
        std::filesystem::path path_) :

        m_Parent{std::move(parent_)},
        m_OsimPath{std::move(path_)},
        m_LoadingResult{std::async(std::launch::async, osc::LoadOsimIntoUndoableModel, m_OsimPath)}
    {
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return "LoadingTab";
    }

    void onTick()
    {
        float const dt = osc::App::get().getDeltaSinceLastFrame().count();

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
            log::info("LoadingScreen::onTick: exception thrown while loading model: %s", ex.what());
            m_LoadingErrorMsg = ex.what();
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
            m_Parent.lock()->addAndSelectTab<ModelEditorTab>(m_Parent, std::move(result));
            m_Parent.lock()->closeTab(m_TabID);
        }
    }

    void onDraw()
    {
        glm::vec2 constexpr menuDims = {512.0f, 512.0f};

        Rect const tabRect = osc::GetMainViewportWorkspaceScreenRect();
        glm::vec2 const windowDims = osc::Dimensions(tabRect);

        // center the menu
        {
            glm::vec2 const menuTopLeft = (windowDims - menuDims) / 2.0f;
            ImGui::SetNextWindowPos(menuTopLeft);
            ImGui::SetNextWindowSize({menuDims.x, -1.0f});
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
                    m_Parent.lock()->addAndSelectTab<LoadingTab>(m_Parent, m_OsimPath);
                    m_Parent.lock()->closeTab(m_TabID);
                }
            }
            ImGui::End();
        }
    }


private:
    UID m_TabID;
    std::weak_ptr<MainUIStateAPI> m_Parent;

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


// public API (PIMPL)

osc::LoadingTab::LoadingTab(
    std::weak_ptr<MainUIStateAPI> parent_,
    std::filesystem::path path_) :

    m_Impl{std::make_unique<Impl>(std::move(parent_), std::move(path_))}
{
}

osc::LoadingTab::LoadingTab(LoadingTab&&) noexcept = default;
osc::LoadingTab& osc::LoadingTab::operator=(LoadingTab&&) noexcept = default;
osc::LoadingTab::~LoadingTab() noexcept = default;

osc::UID osc::LoadingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LoadingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LoadingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LoadingTab::implOnDraw()
{
    m_Impl->onDraw();
}
