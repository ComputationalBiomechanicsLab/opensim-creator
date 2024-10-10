#include <OpenSimCreator/UI/LoadingTab.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
#include <OpenSimCreator/Platform/OpenSimCreatorApp.h>
#include <oscar/Platform/Screen.h>
#include <oscar/UI/ui_context.h>

#include <filesystem>

using namespace osc;

namespace
{
    class LoadingTabTestingScreen final : public Screen {
    public:
    private:
        bool impl_on_event(Event& e) final
        {
            if (ui::context::on_event(e)) {
                return true;
            }
            return m_LoadingTab.on_event(e);
        }

        void impl_on_mount() final
        {
            ui::context::init(App::upd());
            m_LoadingTab.on_mount();
        }

        void impl_on_unmount() final
        {
            m_LoadingTab.on_unmount();
            ui::context::shutdown(App::upd());
        }

        void impl_on_tick() final
        {
            m_LoadingTab.on_tick();
        }

        void impl_on_draw() final
        {
            ui::context::on_start_new_frame(App::upd());
            m_LoadingTab.on_draw();
            ui::context::render();

            if (m_LoadingTab.isFinishedLoading() and m_NumFramesAfterLoading-- == 0) {
                App::upd().request_quit();
            }
        }

        size_t m_NumFramesAfterLoading = 2;
        LoadingTab m_LoadingTab{*this, std::filesystem::weakly_canonical(std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "Blank" / "blank.osim")};
    };
}

TEST(LoadingTab, CanKeepRenderingAfterLoadingFile)
{
    OpenSimCreatorApp app;
    app.show<LoadingTabTestingScreen>();
}
