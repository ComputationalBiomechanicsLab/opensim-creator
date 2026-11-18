#include "loading_tab.h"

#include <libopensimcreator/tests/testopensimcreatorconfig.h>
#include <libopensimcreator/platform/open_sim_creator_app.h>

#include <gtest/gtest.h>
#include <liboscar/platform/widget.h>
#include <liboscar/ui/oscimgui.h>

#include <filesystem>

using namespace osc;

namespace
{
    class LoadingTabTestingScreen final : public Widget {
    public:
    private:
        bool impl_on_event(Event& e) override
        {
            if (ui_context_.on_event(e)) {
                return true;
            }
            return loading_tab_.on_event(e);
        }

        void impl_on_mount() override { loading_tab_.on_mount(); }

        void impl_on_unmount() override { loading_tab_.on_unmount(); }

        void impl_on_tick() override { loading_tab_.on_tick(); }

        void impl_on_draw() override
        {
            ui_context_.on_start_new_frame();
            loading_tab_.on_draw();
            ui_context_.render();

            if (loading_tab_.is_finished_loading() and num_frames_after_loading_-- == 0) {
                App::upd().request_quit();
            }
        }

        ui::Context ui_context_{App::upd()};
        size_t num_frames_after_loading_ = 2;
        LoadingTab loading_tab_{
            this,
            std::filesystem::weakly_canonical(
                std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "Blank" /
                "blank.osim")};
    };
}

TEST(LoadingTab, CanKeepRenderingAfterLoadingFile)
{
    OpenSimCreatorApp app;
    app.show<LoadingTabTestingScreen>();
}
