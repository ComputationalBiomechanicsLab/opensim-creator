#include "LoadingTab.h"

#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/Platform/RecentFiles.h>
#include <libopensimcreator/UI/ModelEditor/ModelEditorTab.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/UI/Events/CloseTabEvent.h>
#include <liboscar/UI/Events/OpenTabEvent.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Tabs/TabPrivate.h>

#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <utility>

using namespace osc;

namespace
{
    std::unique_ptr<UndoableModelStatePair> LoadOsimIntoUndoableModel(const std::filesystem::path& p)
    {
        return std::make_unique<UndoableModelStatePair>(p);
    }
}

class osc::LoadingTab::Impl final : public TabPrivate {
public:

    explicit Impl(
        LoadingTab& owner,
        Widget* parent_,
        std::filesystem::path path_) :

        TabPrivate{owner, parent_, "LoadingTab"},
        m_OsimPath{std::move(path_)},
        m_LoadingResult{std::async(std::launch::async, LoadOsimIntoUndoableModel, m_OsimPath)}
    {}

    bool isFinishedLoading() const { return m_IsFinishedLoading; }

    void on_tick()
    {
        const auto dt = static_cast<float>(App::get().frame_delta_since_last_frame().count());

        // tick the progress bar up a little bit
        m_LoadingProgress += (dt * (1.0f - m_LoadingProgress))/2.0f;

        // if there's an error, then the result came through (it's an error)
        // and this screen should just continuously show the error until the
        // user decides to transition back
        if (not m_LoadingErrorMsg.empty()) {
            return;
        }

        // if there's no error, but the loading result is invalid, then something
        // has already sucessfully taken the result away (below), but `on_tick` was
        // called again
        if (not m_LoadingResult.valid()) {
            m_LoadingErrorMsg = "attempted to call `on_tick` on the loading screen after loading has finished";
            return;
        }

        // otherwise, poll for the result and catch any exceptions that bubble
        // up from the background thread
        std::unique_ptr<UndoableModelStatePair> result = nullptr;
        try {
            if (m_LoadingResult.wait_for(std::chrono::seconds{0}) == std::future_status::ready) {
                result = m_LoadingResult.get();
            }
        }
        catch (const std::exception& ex) {
            log_error("LoadingScreen::on_tick: exception thrown while loading model: %s", ex.what());
            m_LoadingErrorMsg = ex.what();
            m_IsFinishedLoading = true;
            return;
        }

        // if there was a result (a newly-loaded model), handle it
        if (result) {
            // add newly-loaded model to the "Recent Files" list
            App::singleton<RecentFiles>()->push_back(m_OsimPath);

            // Post relevant "loaded" events to this widget, which should
            // propagate up to something that can handle them.
            App::post_event<OpenTabEvent>(owner(), std::make_unique<ModelEditorTab>(&owner(), std::move(result)));
            App::post_event<CloseTabEvent>(owner(), id());
            m_IsFinishedLoading = true;
        }
    }

    void onDraw()
    {
        const Rect workspaceUIRect = ui::get_main_window_workspace_ui_rect();
        const Vec2 workspaceDims = workspaceUIRect.dimensions();
        const Vec2 menuDimsGuess = {0.3f * workspaceDims.x, 6.0f * ui::get_font_base_size()};

        // center the menu
        {
            const Vec2 menuTopLeft = 0.5f * (workspaceDims - menuDimsGuess);
            ui::set_next_panel_ui_pos(workspaceUIRect.ypd_top_left() + menuTopLeft);
            ui::set_next_panel_size({menuDimsGuess.x, -1.0f});
        }

        if (m_LoadingErrorMsg.empty()) {
            if (ui::begin_panel("Loading Message", nullptr, ui::PanelFlag::NoTitleBar)) {
                ui::draw_text("loading: %s", m_OsimPath.string().c_str());
                ui::draw_progress_bar(m_LoadingProgress);
            }
            ui::end_panel();
        }
        else {
            if (ui::begin_panel("Error Message", nullptr, ui::PanelFlag::NoTitleBar)) {
                ui::draw_text_wrapped("An error occurred while loading the file:");
                ui::draw_vertical_spacer(5.0f/15.0f);
                ui::draw_text_wrapped(m_LoadingErrorMsg);
                ui::draw_vertical_spacer(5.0f/15.0f);

                if (ui::draw_button("try again")) {
                    App::post_event<OpenTabEvent>(*parent(), std::make_unique<LoadingTab>(parent(), m_OsimPath));
                    App::post_event<CloseTabEvent>(*parent(), id());
                }
            }
            ui::end_panel();
        }
    }


private:
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

    // set after the file has either finished loading or there was
    // an error (exception) loading it
    bool m_IsFinishedLoading = false;
};


osc::LoadingTab::LoadingTab(
    Widget* parent_,
    std::filesystem::path path_) :

    Tab{std::make_unique<Impl>(*this, parent_, std::move(path_))}
{}
bool osc::LoadingTab::isFinishedLoading() const { return private_data().isFinishedLoading(); }
void osc::LoadingTab::impl_on_tick() { private_data().on_tick(); }
void osc::LoadingTab::impl_on_draw() { private_data().onDraw(); }
