#include <oscar_learnopengl/LearnOpenGLTabRegistry.hpp>

#include <oscar/oscar.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <memory>

using namespace osc;

namespace {

    class TestRunnerScreen final :
        public std::enable_shared_from_this<TestRunnerScreen>,
        public IScreen,
        public ITabHost {
    public:
        TestRunnerScreen()
        {
            RegisterLearnOpenGLTabs(m_Registry);
        }
    private:
        void implOnMount() override
        {
            ImGuiInit();
            mountNextTab();
        }

        void implOnUnmount() override
        {
            unmountCurrentTab();
            ImGuiShutdown();
        }

        void implOnEvent(SDL_Event const& e) override
        {
            ImGuiOnEvent(e);

            if (m_CurrentTab) {
                m_CurrentTab->onEvent(e);
            }

            if (e.type == SDL_QUIT) {
                App::upd().requestQuit();
            }
        }

        void implOnTick() override
        {
            if (m_CurrentTab) {
                m_CurrentTab->onTick();
            }
            if (App::get().getFrameStartTime() >= m_CurrentTabSwitchingTime) {
                mountNextTab();
            }
        }

        void implOnDraw() override
        {
            App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
            ImGuiNewFrame();
            if (m_CurrentTab) {
                m_CurrentTab->onDraw();
            }
            ImGuiRender();
        }

        UID implAddTab(std::unique_ptr<ITab>) override { return UID{}; }
        void implSelectTab(UID) override {}
        void implCloseTab(UID) override {}
        void implResetImgui() override {}

        void mountNextTab()
        {
            ++m_CurrentIndex;
            if (m_CurrentIndex < m_Registry.size()) {
                if (m_CurrentTab) {
                    m_CurrentTab->onUnmount();
                }
                m_CurrentTab = m_Registry[m_CurrentIndex].createTab(ParentPtr<TestRunnerScreen>{shared_from_this()});
                m_CurrentTab->onMount();
                m_CurrentTabSwitchingTime = App::get().getFrameStartTime() + m_OpenDuration;
                App::upd().setMainWindowSubTitle(m_CurrentTab->getName());
            }
            else {
                App::upd().requestQuit();
            }
        }

        void unmountCurrentTab()
        {
            if (m_CurrentTab) {
                m_CurrentTab->onUnmount();
                m_CurrentTab.reset();
            }
        }

        TabRegistry m_Registry;
        AppClock::duration m_OpenDuration = AppSeconds{0};

        size_t m_CurrentIndex = std::numeric_limits<size_t>::max();
        std::unique_ptr<ITab> m_CurrentTab;
        AppClock::time_point m_CurrentTabSwitchingTime;
    };

    class TestRunnerScreenProxy final : public IScreen {
    public:
    private:
        void implOnMount() override { m_Impl->onMount(); }
        void implOnUnmount() override { m_Impl->onUnmount(); }
        void implOnEvent(SDL_Event const& e) { m_Impl->onEvent(e); }
        void implOnTick() override { m_Impl->onTick(); }
        void implOnDraw() override { m_Impl->onDraw(); }

        std::shared_ptr<TestRunnerScreen> m_Impl = std::make_shared<TestRunnerScreen>();
    };
}

TEST(AllRegisteredLearnOpenGLTabs, CanBeRenderedWithNoProblem)
{
    App app{AppMetadata{"someorg", "testoscar_learnopengl"}};
    app.show<TestRunnerScreenProxy>();
}
