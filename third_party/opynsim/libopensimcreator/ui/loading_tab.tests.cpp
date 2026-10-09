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
        bool impl_on_event(Event& e) final
        {
            if (m_UiContext.on_event(e)) {
                return true;
            }
            return m_LoadingTab.on_event(e);
        }

        void impl_on_mount() final
        {
            m_LoadingTab.on_mount();
        }

        void impl_on_unmount() final
        {
            m_LoadingTab.on_unmount();
        }

        void impl_on_tick() final
        {
            m_LoadingTab.on_tick();
        }

        void impl_on_draw() final
        {
            m_UiContext.on_start_new_frame();
            m_LoadingTab.on_draw();
            m_UiContext.render();

            if (m_LoadingTab.isFinishedLoading() and m_NumFramesAfterLoading-- == 0) {
                App::upd().request_quit();
            }
        }

        ui::Context m_UiContext{App::upd()};
        size_t m_NumFramesAfterLoading = 2;
        LoadingTab m_LoadingTab{this, std::filesystem::weakly_canonical(std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "Blank" / "blank.osim")};
    };
}

TEST(LoadingTab, CanKeepRenderingAfterLoadingFile)
{
    OpenSimCreatorApp app;
    app.show<LoadingTabTestingScreen>();
}
