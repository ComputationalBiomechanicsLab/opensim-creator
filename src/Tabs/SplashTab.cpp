#include "SplashTab.hpp"

#include "osc_config.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Graphics/TextureFilterMode.hpp"
#include "src/Graphics/SceneRenderer.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/RecentFile.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Tabs/LoadingTab.hpp"
#include "src/Tabs/MeshImporterTab.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Widgets/MainMenu.hpp"
#include "src/Widgets/LogViewer.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <nonstd/span.hpp>

#include <filesystem>
#include <string>
#include <utility>

static osc::PolarPerspectiveCamera GetSplashScreenDefaultPolarCamera()
{
    osc::PolarPerspectiveCamera rv;
    rv.phi = osc::fpi4/1.5f;
    rv.radius = 10.0f;
    rv.theta = osc::fpi4;
    return rv;
}

static osc::SceneRendererParams GetSplashScreenDefaultRenderParams(osc::PolarPerspectiveCamera const& camera)
{
    osc::SceneRendererParams rv;
    rv.drawRims = false;
    rv.viewMatrix = camera.getViewMtx();
    rv.nearClippingPlane = camera.znear;
    rv.farClippingPlane = camera.zfar;
    rv.viewPos = camera.getPos();
    rv.lightDirection = {-0.34f, -0.25f, 0.05f};
    rv.lightColor = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    rv.backgroundColor = {0.89f, 0.89f, 0.89f, 1.0f};
    return rv;
}

class osc::SplashTab::Impl final {
public:
    Impl(MainUIStateAPI* parent) : m_Parent{std::move(parent)}
    {
        m_OscLogo.setFilterMode(osc::TextureFilterMode::Linear);
        m_CziLogo.setFilterMode(osc::TextureFilterMode::Linear);
        m_TudLogo.setFilterMode(osc::TextureFilterMode::Linear);
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
    }

    void onUnmount()
    {
        App::upd().makeMainEventLoopPolling();
    }

    bool onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_DROPFILE && e.drop.file != nullptr && CStrEndsWith(e.drop.file, ".osim"))
        {
            UID tabID = m_Parent->addTab<LoadingTab>(m_Parent, e.drop.file);
            m_Parent->selectTab(tabID);
            return true;
        }
        return false;
    }

    void onTick()
    {
    }

    void drawMainMenu()
    {
        m_MainMenuFileTab.draw(m_Parent);
        m_MainMenuAboutTab.draw();
    }

    void onDraw()
    {
        if (Area(getTabScreenRect()) > 0.0f)
        {
            drawBackground();
            drawLogo();
            drawMenu();
            drawTUDLogo();
            drawCZLogo();
            drawVersionInfo();
        }
    }

private:
    Rect getTabScreenRect()
    {
        return osc::GetMainViewportWorkspaceScreenRect();
    }

    Rect getMainMenuRect()
    {
        constexpr glm::vec2 menuDims = {700.0f, 500.0f};

        Rect tabRect = getTabScreenRect();
        
        Rect rv{};
        rv.p1 = tabRect.p1 + (Dimensions(tabRect) - menuDims) / 2.0f;
        rv.p2 = rv.p1 + menuDims;
        return rv;
    }

    void drawBackground()
    {
        Rect screenRect = getTabScreenRect();

        ImGui::SetNextWindowPos(screenRect.p1);
        ImGui::SetNextWindowSize(Dimensions(screenRect));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
        ImGui::Begin("##splashscreenbackground", nullptr, GetMinimalWindowFlags());
        ImGui::PopStyleVar();

        SceneRendererParams params{m_LastSceneRendererParams};
        params.dimensions = Dimensions(screenRect);
        params.samples = App::get().getMSXAASamplesRecommended();
        params.projectionMatrix = m_Camera.getProjMtx(AspectRatio(screenRect));

        if (params != m_LastSceneRendererParams)
        {
            m_SceneRenderer.draw({}, params);
            m_LastSceneRendererParams = params;
        }

        osc::DrawTextureAsImGuiImage(m_SceneRenderer.updRenderTexture(), m_SceneRenderer.getDimensions());

        ImGui::End();
    }

    void drawLogo()
    {
        constexpr glm::vec2 logoDims = {128.0f, 128.0f};
        constexpr float padding = 25.0f;

        Rect mmr = getMainMenuRect();       

        glm::vec2 loc
        {
            mmr.p1.x + Dimensions(mmr).x/2.0f - logoDims.x/2.0f,
            mmr.p1.y - padding - logoDims.y
        };

        ImGui::SetNextWindowPos(loc);
        ImGui::Begin("##osclogo", nullptr, GetMinimalWindowFlags());
        osc::DrawTextureAsImGuiImage(m_OscLogo, logoDims);
        ImGui::End();
    }

    void drawMenu()
    {
        {
            Rect mmr = getMainMenuRect();
            glm::vec2 mmrDims = Dimensions(mmr);
            ImGui::SetNextWindowPos(mmr.p1);
            ImGui::SetNextWindowSize(ImVec2(Dimensions(mmr).x, -1));
            ImGui::SetNextWindowSizeConstraints(Dimensions(mmr), Dimensions(mmr));
        }        

        if (ImGui::Begin("Splash screen", nullptr, ImGuiWindowFlags_NoTitleBar))
        {
            // `import meshes` button
            {
                ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, OSC_POSITIVE_HOVERED_RGBA);
                if (ImGui::Button(ICON_FA_MAGIC " Import Meshes"))
                {
                    UID tabID = m_Parent->addTab<MeshImporterTab>(m_Parent);
                    m_Parent->selectTab(tabID);
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
                    ActionNewModel(m_Parent);
                }
                ImGui::PopStyleColor(2);
            }

            ImGui::SameLine();

            // `open` button
            if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open Model (Ctrl+O)"))
            {
                ActionOpenModel(m_Parent);
            }

            ImGui::SameLine();

            // `docs` button
            if (ImGui::Button(ICON_FA_BOOK " Open Documentation"))
            {
                OpenPathInOSDefaultApplication(osc::App::get().getConfig().getHTMLDocsDir() / "index.html");
            }

            ImGui::Dummy({0.0f, 10.0f});

            // de-dupe imgui IDs because these lists may contain duplicate
            // names
            int id = 0;

            ImGui::Columns(2);

            // left column: recent files
            ImGui::TextUnformatted("Recent files:");
            ImGui::Dummy({0.0f, 3.0f});

            if (!m_MainMenuFileTab.recentlyOpenedFiles.empty())
            {
                // iterate in reverse: recent files are stored oldest --> newest
                for (auto it = m_MainMenuFileTab.recentlyOpenedFiles.rbegin(); it != m_MainMenuFileTab.recentlyOpenedFiles.rend(); ++it)
                {
                    RecentFile const& rf = *it;
                    ImGui::PushID(++id);
                    if (ImGui::Button(rf.path.filename().string().c_str()))
                    {
                        UID tabID = m_Parent->addTab<LoadingTab>(m_Parent, rf.path);
                        m_Parent->selectTab(tabID);
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
            if (!m_MainMenuFileTab.exampleOsimFiles.empty())
            {
                ImGui::TextUnformatted("Example files:");
                ImGui::Dummy({0.0f, 3.0f});

                for (std::filesystem::path const& ex : m_MainMenuFileTab.exampleOsimFiles)
                {
                    ImGui::PushID(++id);
                    if (ImGui::Button(ex.filename().string().c_str()))
                    {
                        UID tabID = m_Parent->addTab<LoadingTab>(m_Parent, ex);
                        m_Parent->selectTab(tabID);
                    }
                    ImGui::PopID();
                }
            }
            ImGui::NextColumn();

            ImGui::Columns();
        }
        ImGui::End();
    }

    void drawTUDLogo()
    {
        constexpr glm::vec2 logoDims = {128.0f, 128.0f};
        constexpr float padding = 25.0f;

        Rect mmr = getMainMenuRect();

        glm::vec2 loc
        {
            (mmr.p1.x + mmr.p2.x)/2.0f - padding - logoDims.x,
            mmr.p2.y + padding,
        };

        ImGui::SetNextWindowPos(loc);
        ImGui::Begin("##tudlogo", nullptr, GetMinimalWindowFlags());
        osc::DrawTextureAsImGuiImage(m_TudLogo, logoDims);
        ImGui::End();
    }

    void drawCZLogo()
    {
        constexpr glm::vec2 logoDims = {128.0f, 128.0f};
        constexpr float padding = 25.0f;

        Rect mmr = getMainMenuRect();

        glm::vec2 loc
        {
            (mmr.p1.x + mmr.p2.x)/2.0f + padding,
            mmr.p2.y + padding,
        };

        ImGui::SetNextWindowPos(loc);
        ImGui::Begin("##czlogo", nullptr, GetMinimalWindowFlags());
        osc::DrawTextureAsImGuiImage(m_CziLogo, logoDims);
        ImGui::End();
    }

    void drawVersionInfo()
    {
        Rect tabRect = getTabScreenRect();
        float h = ImGui::GetTextLineHeightWithSpacing();
        constexpr float padding = 5.0f;

        glm::vec2 pos{};
        pos.x = tabRect.p1.x + padding;
        pos.y = tabRect.p2.y - h - padding;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
        char const* content = "OpenSim Creator v" OSC_VERSION_STRING " (build " OSC_BUILD_ID ")";
        dl->AddText(pos, color, content);
    }

    // tab unique ID
    UID m_ID;

    // tab name
    std::string m_Name = ICON_FA_HOME;

    // tab parent
    MainUIStateAPI* m_Parent;

    // for rendering the 3D scene
    osc::PolarPerspectiveCamera m_Camera = GetSplashScreenDefaultPolarCamera();
    SceneRenderer m_SceneRenderer;
    SceneRendererParams m_LastSceneRendererParams = GetSplashScreenDefaultRenderParams(m_Camera);

    // main app logo, blitted to top of the screen
    Texture2D m_OscLogo = LoadTexture2DFromImageResource("logo.png", ImageFlags_FlipVertically);

    // CZI attributation logo, blitted to bottom of screen
    Texture2D m_CziLogo = LoadTexture2DFromImageResource("chanzuckerberg_logo.png", ImageFlags_FlipVertically);

    // TUD attributation logo, blitted to bottom of screen
    Texture2D m_TudLogo = LoadTexture2DFromImageResource("tud_logo.png", ImageFlags_FlipVertically);

    // main menu (top bar) states
    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;

    LogViewer m_LogViewer;
};


// public API

osc::SplashTab::SplashTab(MainUIStateAPI* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::SplashTab::SplashTab(SplashTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SplashTab& osc::SplashTab::operator=(SplashTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::SplashTab::~SplashTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::SplashTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::SplashTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::SplashTab::implParent() const
{
    return m_Impl->parent();
}

void osc::SplashTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::SplashTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::SplashTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::SplashTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::SplashTab::implOnDrawMainMenu()
{
    m_Impl->drawMainMenu();
}

void osc::SplashTab::implOnDraw()
{
    m_Impl->onDraw();
}
