#include "SplashScreen.hpp"

#include "osc_config.hpp"

#include "src/Graphics/Shaders/GouraudShader.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/Texturing.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Screens/LoadingScreen.hpp"
#include "src/Screens/MeshImporterScreen.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Widgets/MainMenu.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include <filesystem>
#include <memory>

static gl::Texture2D LoadImageResourceIntoTexture(char const* res_pth)
{
    return osc::loadImageAsTexture(osc::App::resource(res_pth).string().c_str()).texture;
}

static glm::mat4x3 GenerateFloorModelMatrix()
{
    // rotate from XY (+Z dir) to ZY (+Y dir)
    glm::mat4 rv = glm::rotate(glm::mat4{1.0f}, -osc::fpi2, {1.0f, 0.0f, 0.0f});

    // make floor extend far in all directions
    rv = glm::scale(glm::mat4{1.0f}, {100.0f, 1.0f, 100.0f}) * rv;

    // lower slightly, so that it doesn't conflict with OpenSim model planes
    // that happen to lie at Z==0
    rv = glm::translate(glm::mat4{1.0f}, {0.0f, -0.0001f, 0.0f}) * rv;

    return glm::mat4x3{rv};
}

class osc::SplashScreen::Impl final {
public:
    Impl() :
        Impl{nullptr}
    {
    }

    Impl(std::shared_ptr<MainEditorState> mes_) :
        maybeMainEditorState{std::move(mes_)}
    {
        camera.phi = fpi4/1.5f;
        camera.radius = 10.0f;
        camera.theta = fpi4;
    }

    void onMount()
    {
        osc::ImGuiInit();
        App::cur().makeMainEventLoopWaiting();
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
        App::cur().makeMainEventLoopPolling();
    }

    void onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_QUIT)
        {
            App::cur().requestQuit();
            return;
        }
        else if (osc::ImGuiOnEvent(e))
        {
            return;
        }
        else if (e.type == SDL_DROPFILE && e.drop.file != nullptr && CStrEndsWith(e.drop.file, ".osim"))
        {
            App::cur().requestTransition<LoadingScreen>(maybeMainEditorState, e.drop.file);
            return;
        }
    }

    void tick(float)
    {
    }

    void draw()
    {
        constexpr glm::vec2 menuDims = {700.0f, 500.0f};

        App& app = App::cur();

        glm::vec2 windowDims = app.dims();
        gl::Viewport(0, 0, app.idims().x, app.idims().y);
        gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        osc::ImGuiNewFrame();

        {
            GouraudShader& s = gouraud;
            gl::UseProgram(s.program);

            gl::Uniform(s.uProjMat, camera.getProjMtx(app.aspectRatio()));
            gl::Uniform(s.uViewMat, camera.getViewMtx());
            gl::Uniform(s.uModelMat, floorMat);
            gl::Uniform(s.uNormalMat, floorNormalMat);
            gl::Uniform(s.uDiffuseColor, {1.0f, 1.0f, 1.0f, 1.0f});
            gl::Uniform(s.uLightDir, lightDir);
            gl::Uniform(s.uLightColor, lightCol);
            gl::Uniform(s.uViewPos, camera.getPos());
            gl::Uniform(s.uIsTextured, true);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(chequer);
            gl::Uniform(s.uSampler0, gl::textureIndex<GL_TEXTURE0>());
            gl::BindVertexArray(floorMesh->GetVertexArray());
            floorMesh->Draw();
            gl::BindVertexArray();
        }

        if (ImGui::BeginMainMenuBar())
        {
            mmFileTab.draw(maybeMainEditorState);
            mmAboutTab.draw();
            ImGui::EndMainMenuBar();
        }

        ImGuiWindowFlags imgFlags =
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoDecoration;

        constexpr glm::vec2 logoDims = {128.0f, 128.0f};
        constexpr float padding = 25.0f;

        // draw logo just above the menu
        {
            glm::vec2 loc = windowDims/2.0f;
            loc.y -= menuDims.y/2.0f;
            loc.y -= padding;  // padding
            loc.y -= logoDims.y;
            loc.x -= logoDims.x/2.0f;

            ImGui::SetNextWindowPos(loc);
            ImGui::Begin("logowindow", nullptr, imgFlags);
            ImGui::Image(logo.getVoidHandle(), logoDims);
            ImGui::End();
        }

        // center the menu
        {
            glm::vec2 menuPos = (windowDims - menuDims) / 2.0f;
            ImGui::SetNextWindowPos(menuPos);
            ImGui::SetNextWindowSize(ImVec2(menuDims.x, -1));
            ImGui::SetNextWindowSizeConstraints(menuDims, menuDims);
        }

        if (ImGui::Begin("Splash screen", nullptr, ImGuiWindowFlags_NoTitleBar))
        {
            // `import meshes` button
            {
                ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, OSC_POSITIVE_HOVERED_RGBA);
                if (ImGui::Button(ICON_FA_MAGIC " Import Meshes"))
                {
                    App::cur().requestTransition<MeshImporterScreen>();
                }
                ImGui::PopStyleColor(2);
            }

            ImGui::SameLine();

            // `new` button
            {
                ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, OSC_POSITIVE_HOVERED_RGBA);
                if (ImGui::Button(ICON_FA_FILE_ALT " New Model (Ctrl+N)"))
                {
                    actionNewModel(maybeMainEditorState);
                }
                ImGui::PopStyleColor(2);
            }

            ImGui::SameLine();

            // `open` button
            if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open Model (Ctrl+O)"))
            {
                actionOpenModel(maybeMainEditorState);
            }

            ImGui::SameLine();

            // `docs` button
            if (ImGui::Button(ICON_FA_BOOK " Open Documentation"))
            {
                OpenPathInOSDefaultApplication(App::config().getHTMLDocsDir() / "index.html");
            }

            ImGui::Dummy(ImVec2{0.0f, 10.0f});

            // de-dupe imgui IDs because these lists may contain duplicate
            // names
            int id = 0;

            ImGui::Columns(2);

            // left column: recent files
            ImGui::TextUnformatted("Recent files:");
            ImGui::Dummy(ImVec2{0.0f, 3.0f});

            if (!mmFileTab.recentlyOpenedFiles.empty())
            {
                // iterate in reverse: recent files are stored oldest --> newest
                for (auto it = mmFileTab.recentlyOpenedFiles.rbegin(); it != mmFileTab.recentlyOpenedFiles.rend(); ++it)
                {
                    RecentFile const& rf = *it;
                    ImGui::PushID(++id);
                    if (ImGui::Button(rf.path.filename().string().c_str()))
                    {
                        app.requestTransition<osc::LoadingScreen>(maybeMainEditorState, rf.path);
                    }
                    ImGui::PopID();
                }
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OSC_GREYED_RGBA);
                ImGui::TextWrapped("No files opened recently. Try:");
                ImGui::BulletText("Creating a new model (Ctrl+N)");
                ImGui::BulletText("Opening an existing model (Ctrl+O)");
                ImGui::BulletText("Opening an example (right-side)");
                ImGui::PopStyleColor();
            }
            ImGui::NextColumn();

            // right column: example model files
            if (!mmFileTab.exampleOsimFiles.empty())
            {
                ImGui::TextUnformatted("Example files:");
                ImGui::Dummy(ImVec2{0.0f, 3.0f});

                for (std::filesystem::path const& ex : mmFileTab.exampleOsimFiles)
                {
                    ImGui::PushID(++id);
                    if (ImGui::Button(ex.filename().string().c_str()))
                    {
                        app.requestTransition<LoadingScreen>(maybeMainEditorState, ex);
                    }
                    ImGui::PopID();
                }
            }
            ImGui::NextColumn();

            ImGui::Columns();
        }
        ImGui::End();

        // draw TUD logo below menu, slightly to the left
        {
            glm::vec2 loc = windowDims/2.0f;
            loc.y += menuDims.y/2.0f;
            loc.y += padding;
            loc.x -= padding;
            loc.x -= logoDims.x;

            ImGui::SetNextWindowPos(loc);
            ImGui::Begin("##tudlogo", nullptr, imgFlags);
            ImGui::Image(tudLogo.getVoidHandle(), logoDims);
            ImGui::End();
        }

        // draw CZI logo below menu, slightly to the right
        {
            glm::vec2 loc = windowDims/2.0f;
            loc.y += menuDims.y/2.0f;
            loc.y += padding;
            loc.x += padding;

            ImGui::SetNextWindowPos(loc);
            ImGui::Begin("##czilogo", nullptr, imgFlags);
            ImGui::Image(czLogo.getVoidHandle(), logoDims);
            ImGui::End();
        }

        // draw version information
        {
            float h = ImGui::GetTextLineHeightWithSpacing();
            glm::vec2 pos{0.0f, windowDims.y - h};
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            char const* content = "OpenSim Creator v" OSC_VERSION_STRING " (build " OSC_BUILD_ID ")";
            dl->AddText(pos, color, content);
        }

        osc::ImGuiRender();
    }

private:

    // used to render floor
    osc::GouraudShader& gouraud = App::shader<GouraudShader>();

    glm::vec3 lightDir = {-0.34f, -0.25f, 0.05f};
    glm::vec3 lightCol = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    glm::vec4 backgroundCol = {0.89f, 0.89f, 0.89f, 1.0f};

    std::shared_ptr<Mesh> floorMesh = App::meshes().getFloorMesh();

    glm::mat4 floorMat = GenerateFloorModelMatrix();
    glm::mat4 floorNormalMat = ToNormalMatrix(floorMat);

    // floor chequer texture
    gl::Texture2D chequer = genChequeredFloorTexture();

    // main app logo, blitted to top of the screen
    gl::Texture2D logo = LoadImageResourceIntoTexture("logo.png");

    // CZI attributation logo, blitted to bottom of screen
    gl::Texture2D czLogo = LoadImageResourceIntoTexture("chanzuckerberg_logo.png");

    // TUD attributation logo, blitted to bottom of screen
    gl::Texture2D tudLogo = LoadImageResourceIntoTexture("tud_logo.png");

    // camera for top-down shot of the floor
    PolarPerspectiveCamera camera;

    // main menu (top bar) states
    MainMenuFileTab mmFileTab;
    MainMenuAboutTab mmAboutTab;

    // top-level UI state that's shared between screens (can be null)
    std::shared_ptr<MainEditorState> maybeMainEditorState;
};


// public API (PIMPL)

osc::SplashScreen::SplashScreen() :
    m_Impl{new Impl{}}
{
}

osc::SplashScreen::SplashScreen(std::shared_ptr<MainEditorState> mes_) :
    m_Impl{new Impl{std::move(mes_)}}
{
}

osc::SplashScreen::SplashScreen(SplashScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SplashScreen& osc::SplashScreen::operator=(SplashScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::SplashScreen::~SplashScreen() noexcept
{
    delete m_Impl;
}

void osc::SplashScreen::onMount()
{
    m_Impl->onMount();
}

void osc::SplashScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::SplashScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::SplashScreen::tick(float dt)
{
    m_Impl->tick(std::move(dt));
}

void osc::SplashScreen::draw()
{
    m_Impl->draw();
}
