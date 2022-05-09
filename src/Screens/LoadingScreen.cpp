#include "LoadingScreen.hpp"

#include "src/OpenSimBindings/MainEditorState.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Screens/ModelEditorScreen.hpp"
#include "src/Screens/SplashScreen.hpp"
#include "src/Utils/Assertions.hpp"

#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_events.h>

#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>


// the function that loads the OpenSim model
static std::unique_ptr<osc::UndoableModelStatePair> loadOpenSimModel(std::string path)
{
    auto model = std::make_unique<OpenSim::Model>(path);
    return std::make_unique<osc::UndoableModelStatePair>(std::move(model));
}

class osc::LoadingScreen::Impl final {
public:

    explicit Impl(std::filesystem::path osimPath) :
        Impl{std::make_shared<osc::MainEditorState>(), std::move(osimPath)}
    {
    }

    Impl(std::shared_ptr<MainEditorState> state, std::filesystem::path osimPath) :

        // save the path being loaded
        m_OsimPath{std::move(osimPath)},

        // immediately start loading the model file on a background thread
        m_LoadingResult{std::async(std::launch::async, loadOpenSimModel, m_OsimPath.string())},

        // save the editor state (if any): it will be forwarded after loading the model
        m_MainEditorState{std::move(state)}
    {
        if (!m_MainEditorState)
        {
            m_MainEditorState = std::make_shared<MainEditorState>();
        }

        OSC_ASSERT(m_MainEditorState);
    }

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
            return;
        }
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            App::upd().requestTransition<SplashScreen>();
            return;
        }
    }

    void tick(float dt)
    {
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
            *m_MainEditorState->editedModel() = std::move(*result);
            m_MainEditorState->editedModel()->setUpToDateWithFilesystem();
            App::upd().requestTransition<ModelEditorScreen>(m_MainEditorState);
            AutoFocusAllViewers(*m_MainEditorState);
        }
    }

    void draw()
    {
        osc::ImGuiNewFrame();

        constexpr glm::vec2 menu_dims = {512.0f, 512.0f};

        App::upd().clearScreen({0.99f, 0.98f, 0.96f, 1.0f});

        glm::vec2 window_dims = App::get().dims();

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
                ImGui::Dummy(ImVec2{0.0f, 5.0f});
                ImGui::TextWrapped("%s", m_LoadingErrorMsg.c_str());
                ImGui::Dummy(ImVec2{0.0f, 5.0f});

                if (ImGui::Button("back to splash screen (ESC)"))
                {
                    App::upd().requestTransition<SplashScreen>();
                }
                ImGui::SameLine();
                if (ImGui::Button("try again"))
                {
                    App::upd().requestTransition<LoadingScreen>(m_MainEditorState, m_OsimPath);
                }
            }
            ImGui::End();
        }
        osc::ImGuiRender();
    }

private:


    // filesystem path to the osim being loaded
    std::filesystem::path m_OsimPath;

    // future that lets the UI thread poll the loading thread for
    // the loaded model
    std::future<std::unique_ptr<osc::UndoableModelStatePair>> m_LoadingResult;

    // if not empty, any error encountered by the loading thread
    std::string m_LoadingErrorMsg;

    // a main state that should be recycled by this screen when
    // transitioning into the editor
    std::shared_ptr<MainEditorState> m_MainEditorState;

    // a fake progress indicator that never quite reaches 100 %
    //
    // this might seem evil, but its main purpose is to ensure the
    // user that *something* is happening - even if that "something"
    // is "the background thread is deadlocked" ;)
    float m_LoadingProgress = 0.0f;
};


// public API (PIMPL)

osc::LoadingScreen::LoadingScreen(std::filesystem::path osimPath) :
    m_Impl{new Impl{std::move(osimPath)}}
{
}

osc::LoadingScreen::LoadingScreen(
        std::shared_ptr<MainEditorState> state,
        std::filesystem::path osimPath) :

    m_Impl{new Impl{std::move(state), std::move(osimPath)}}
{
}

osc::LoadingScreen::LoadingScreen(LoadingScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::LoadingScreen& osc::LoadingScreen::operator=(LoadingScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::LoadingScreen::~LoadingScreen() noexcept
{
    delete m_Impl;
}

void osc::LoadingScreen::onMount()
{
    m_Impl->onMount();
}

void osc::LoadingScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::LoadingScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::LoadingScreen::tick(float dt)
{
    m_Impl->tick(std::move(dt));
}

void osc::LoadingScreen::draw()
{
    m_Impl->draw();
}
