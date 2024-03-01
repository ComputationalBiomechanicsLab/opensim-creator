#include "LoadingTab.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Platform/RecentFiles.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorTab.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/Utils/ParentPtr.h>

#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <utility>

using namespace osc;

class osc::LoadingTab::Impl final {
public:

    Impl(
        ParentPtr<IMainUIStateAPI> const& parent_,
        std::filesystem::path path_) :

        m_Parent{parent_},
        m_OsimPath{std::move(path_)},
        m_LoadingResult{std::async(std::launch::async, LoadOsimIntoUndoableModel, m_OsimPath)}
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
        auto const dt = static_cast<float>(App::get().getFrameDeltaSinceLastFrame().count());

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
            log_info("LoadingScreen::onTick: exception thrown while loading model: %s", ex.what());
            m_LoadingErrorMsg = ex.what();
            return;
        }

        // if there was a result (a newly-loaded model), handle it
        if (result)
        {
            // add newly-loaded model to the "Recent Files" list
            App::singleton<RecentFiles>()->push_back(m_OsimPath);

            // there is an existing editor state
            //
            // recycle it so that users can keep their running sims, local edits, etc.
            m_Parent->addAndSelectTab<ModelEditorTab>(m_Parent, std::move(result));
            m_Parent->closeTab(m_TabID);
        }
    }

    void onDraw()
    {
        constexpr Vec2 menuDims = {512.0f, 512.0f};

        Rect const tabRect = ui::GetMainViewportWorkspaceScreenRect();
        Vec2 const windowDims = dimensions(tabRect);

        // center the menu
        {
            Vec2 const menuTopLeft = (windowDims - menuDims) / 2.0f;
            ui::SetNextWindowPos(menuTopLeft);
            ui::SetNextWindowSize({menuDims.x, -1.0f});
        }

        if (m_LoadingErrorMsg.empty())
        {
            if (ui::Begin("Loading Message", nullptr, ImGuiWindowFlags_NoTitleBar))
            {
                ui::Text("loading: %s", m_OsimPath.string().c_str());
                ui::ProgressBar(m_LoadingProgress);
            }
            ui::End();
        }
        else
        {
            if (ui::Begin("Error Message", nullptr, ImGuiWindowFlags_NoTitleBar))
            {
                ui::TextWrapped("An error occurred while loading the file:");
                ui::Dummy({0.0f, 5.0f});
                ui::TextWrapped(m_LoadingErrorMsg);
                ui::Dummy({0.0f, 5.0f});

                if (ui::Button("try again"))
                {
                    m_Parent->addAndSelectTab<LoadingTab>(m_Parent, m_OsimPath);
                    m_Parent->closeTab(m_TabID);
                }
            }
            ui::End();
        }
    }


private:
    UID m_TabID;
    ParentPtr<IMainUIStateAPI> m_Parent;

    // filesystem path to the osim being loaded
    std::filesystem::path m_OsimPath;

    // future that lets the UI thread poll the loading thread for
    // the loaded model
    std::future<std::unique_ptr<UndoableModelStatePair>> m_LoadingResult;

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
    ParentPtr<IMainUIStateAPI> const& parent_,
    std::filesystem::path path_) :

    m_Impl{std::make_unique<Impl>(parent_, std::move(path_))}
{
}

osc::LoadingTab::LoadingTab(LoadingTab&&) noexcept = default;
osc::LoadingTab& osc::LoadingTab::operator=(LoadingTab&&) noexcept = default;
osc::LoadingTab::~LoadingTab() noexcept = default;

UID osc::LoadingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LoadingTab::implGetName() const
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
