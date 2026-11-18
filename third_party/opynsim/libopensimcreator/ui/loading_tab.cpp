#include "loading_tab.h"

#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/platform/recent_files.h>
#include <libopensimcreator/ui/model_editor/model_editor_tab.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/events/close_tab_event.h>
#include <liboscar/ui/events/open_tab_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/tab_private.h>

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
    std::unique_ptr<UndoableModelStatePair> load_osim_into_undoable_model(
        const std::filesystem::path& p)
    {
        return std::make_unique<UndoableModelStatePair>(p);
    }
}

class osc::LoadingTab::Impl final : public TabPrivate {
public:
    explicit Impl(LoadingTab& owner, Widget* parent, std::filesystem::path path) :

        TabPrivate{owner, parent, "LoadingTab"},
        osim_path_{std::move(path)},
        loading_result_{std::async(std::launch::async, load_osim_into_undoable_model, osim_path_)}
    {}

    bool is_finished_loading() const { return is_finished_loading_; }

    void on_tick()
    {
        const auto dt = static_cast<float>(App::get().frame_delta_since_last_frame().count());

        // tick the progress bar up a little bit
        loading_progress_ += (dt * (1.0f - loading_progress_)) / 2.0f;

        // if there's an error, then the result came through (it's an error)
        // and this screen should just continuously show the error until the
        // user decides to transition back
        if (not loading_error_msg_.empty()) {
            return;
        }

        // if there's no error, but the loading result is invalid, then something
        // has already sucessfully taken the result away (below), but `on_tick` was
        // called again
        if (not loading_result_.valid()) {
            loading_error_msg_ =
                "attempted to call `on_tick` on the loading screen after loading has finished";
            return;
        }

        // otherwise, poll for the result and catch any exceptions that bubble
        // up from the background thread
        std::unique_ptr<UndoableModelStatePair> result = nullptr;
        try {
            if (loading_result_.wait_for(std::chrono::seconds{0}) == std::future_status::ready) {
                result = loading_result_.get();
            }
        } catch (const std::exception& ex) {
            log_error(
                "LoadingScreen::on_tick: exception thrown while loading model: {}", ex.what());
            loading_error_msg_ = ex.what();
            is_finished_loading_ = true;
            return;
        }

        // if there was a result (a newly-loaded model), handle it
        if (result) {
            // add newly-loaded model to the "Recent Files" list
            App::singleton<RecentFiles>()->push_back(osim_path_);

            // Post relevant "loaded" events to this widget, which should
            // propagate up to something that can handle them.
            App::post_event<OpenTabEvent>(
                owner(), std::make_unique<ModelEditorTab>(&owner(), std::move(result)));
            App::post_event<CloseTabEvent>(owner(), id());
            is_finished_loading_ = true;
        }
    }

    void on_draw()
    {
        const Rect workspace_ui_rect = ui::get_main_window_workspace_ui_rect();
        const Vector2 workspace_dims = workspace_ui_rect.dimensions();
        const Vector2 menu_dims_guess = {
            0.3f * workspace_dims.x(), 6.0f * ui::get_font_base_size()};

        // center the menu
        {
            const Vector2 menu_top_left = 0.5f * (workspace_dims - menu_dims_guess);
            ui::set_next_panel_ui_position(workspace_ui_rect.ypd_top_left() + menu_top_left);
            ui::set_next_panel_size({menu_dims_guess.x(), -1.0f});
        }

        if (loading_error_msg_.empty()) {
            if (ui::begin_panel("Loading Message", nullptr, ui::PanelFlag::NoTitleBar)) {
                ui::draw_text("loading: {}", osim_path_.string());
                ui::draw_progress_bar(loading_progress_);
            }
            ui::end_panel();
        } else {
            if (ui::begin_panel("Error Message", nullptr, ui::PanelFlag::NoTitleBar)) {
                ui::draw_text_wrapped("An error occurred while loading the file:");
                ui::draw_vertical_spacer(5.0f / 15.0f);
                ui::draw_text_wrapped(loading_error_msg_);
                ui::draw_vertical_spacer(5.0f / 15.0f);

                if (ui::draw_button("try again")) {
                    App::post_event<OpenTabEvent>(
                        *parent(), std::make_unique<LoadingTab>(parent(), osim_path_));
                    App::post_event<CloseTabEvent>(*parent(), id());
                }
            }
            ui::end_panel();
        }
    }

private:
    // filesystem path to the osim being loaded
    std::filesystem::path osim_path_;

    // future that lets the UI thread poll the loading thread for
    // the loaded model
    std::future<std::unique_ptr<UndoableModelStatePair>> loading_result_;

    // if not empty, any error encountered by the loading thread
    std::string loading_error_msg_;

    // a fake progress indicator that never quite reaches 100 %
    //
    // this might seem evil, but its main purpose is to ensure the
    // user that *something* is happening - even if that "something"
    // is "the background thread is deadlocked" ;)
    float loading_progress_ = 0.0f;

    // set after the file has either finished loading or there was
    // an error (exception) loading it
    bool is_finished_loading_ = false;
};

osc::LoadingTab::LoadingTab(Widget* parent, std::filesystem::path path) :

    Tab{std::make_unique<Impl>(*this, parent, std::move(path))}
{}
bool osc::LoadingTab::is_finished_loading() const
{
    return private_data().is_finished_loading();
}
void osc::LoadingTab::impl_on_tick()
{
    private_data().on_tick();
}
void osc::LoadingTab::impl_on_draw()
{
    private_data().on_draw();
}
