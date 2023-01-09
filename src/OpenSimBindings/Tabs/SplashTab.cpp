#include "SplashTab.hpp"

#include "osc_config.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Formats/SVG.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Graphics/TextureFilterMode.hpp"
#include "src/Graphics/SceneRenderer.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/Widgets/MainMenu.hpp"
#include "src/OpenSimBindings/Tabs/LoadingTab.hpp"
#include "src/OpenSimBindings/Tabs/MeshImporterTab.hpp"
#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/RecentFile.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Utils/Algorithms.hpp"
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
            // if the user drops an osim file on this tab then it should be loaded
            UID const tabID = m_Parent->addTab<LoadingTab>(m_Parent, e.drop.file);
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
        if (Area(osc::GetMainViewportWorkspaceScreenRect()) <= 0.0f)
        {
            // edge-case: splash screen is the first rendered frame and ImGui
            //            is being unusual about it
            return;
        }

        drawBackground();
        drawLogo();
        drawAttributationLogos();
        drawVersionInfo();
        drawMenu();
    }

private:
    Rect calcMainMenuRect() const
    {
        Rect tabRect = osc::GetMainViewportWorkspaceScreenRect();
        tabRect.p2.y -= m_AttributationLogoDims.y + m_AttributationLogoPadding.y;  // pretend the attributation bar isn't there (avoid it)

        glm::vec2 const menuAndTopLogoDims = osc::Min(osc::Dimensions(tabRect), glm::vec2{m_MenuMaxDims.x, m_MenuMaxDims.y + m_TopLogoDims.y + m_TopLogoPadding.y});
        glm::vec2 const menuAndTopLogoTopLeft = tabRect.p1 + 0.5f*(Dimensions(tabRect) - menuAndTopLogoDims);
        glm::vec2 const menuDims = {menuAndTopLogoDims.x, menuAndTopLogoDims.y - m_TopLogoDims.y - m_TopLogoPadding.y};
        glm::vec2 const menuTopLeft = glm::vec2{menuAndTopLogoTopLeft.x, menuAndTopLogoTopLeft.y + m_TopLogoDims.y + m_TopLogoPadding.y};

        return Rect{menuTopLeft, menuTopLeft + menuDims};
    }

    Rect calcLogoRect() const
    {
        Rect const mmr = calcMainMenuRect();
        glm::vec2 const topLeft
        {
            mmr.p1.x + Dimensions(mmr).x/2.0f - m_TopLogoDims.x/2.0f,
            mmr.p1.y - m_TopLogoPadding.y - m_TopLogoDims.y,
        };

        return Rect{topLeft, topLeft + m_TopLogoDims};
    }

    void drawBackground()
    {
        Rect const screenRect = osc::GetMainViewportWorkspaceScreenRect();

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

        osc::DrawTextureAsImGuiImage(m_SceneRenderer.updRenderTexture());

        ImGui::End();
    }

    void drawLogo()
    {
        Rect const logoRect = calcLogoRect();

        ImGui::SetNextWindowPos(logoRect.p1);
        ImGui::Begin("##osclogo", nullptr, GetMinimalWindowFlags());
        osc::DrawTextureAsImGuiImage(m_OscLogo, osc::Dimensions(logoRect));
        ImGui::End();
    }

    void drawMenu()
    {
        {
            Rect const mmr = calcMainMenuRect();
            ImGui::SetNextWindowPos(mmr.p1);
            ImGui::SetNextWindowSize({Dimensions(mmr).x, -1.0f});
            ImGui::SetNextWindowSizeConstraints(Dimensions(mmr), Dimensions(mmr));
        }

        if (ImGui::Begin("Splash screen", nullptr, ImGuiWindowFlags_NoTitleBar))
        {
            ImGui::Columns(2, 0, false);

            // left column: actions and recent files
            {
                ImGui::TextDisabled("Actions");
                ImGui::Dummy({0.0f, 2.0f});

                if (ImGui::MenuItem(ICON_FA_FILE_ALT " New Model"))
                {
                    ActionNewModel(*m_Parent);
                }
                if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Model"))
                {
                    ActionOpenModel(*m_Parent);
                }
                if (ImGui::MenuItem(ICON_FA_MAGIC " Import Meshes"))
                {
                    UID const tabID = m_Parent->addTab<MeshImporterTab>(m_Parent);
                    m_Parent->selectTab(tabID);
                }
                osc::App::upd().addFrameAnnotation("SplashTab/ImportMeshesMenuItem", osc::GetItemRect());
                if (ImGui::MenuItem(ICON_FA_BOOK " Open Documentation"))
                {
                    OpenPathInOSDefaultApplication(osc::App::get().getConfig().getHTMLDocsDir() / "index.html");
                }
            }

            ImGui::Dummy({0.0f, 1.0f*ImGui::GetTextLineHeight()});
            ImGui::TextDisabled("Recent Models");
            ImGui::Dummy({0.0f, 2.0f});

            // de-dupe imgui IDs because these lists may contain duplicate
            // names
            int imguiID = 0;

            if (!m_MainMenuFileTab.recentlyOpenedFiles.empty())
            {
                // iterate in reverse: recent files are stored oldest --> newest
                for (auto it = m_MainMenuFileTab.recentlyOpenedFiles.rbegin(); it != m_MainMenuFileTab.recentlyOpenedFiles.rend(); ++it)
                {
                    RecentFile const& rf = *it;

                    std::string const label = std::string{ICON_FA_FILE} + " " + rf.path.filename().string();

                    ImGui::PushID(++imguiID);
                    if (ImGui::MenuItem(label.c_str()))
                    {
                        UID const tabID = m_Parent->addTab<LoadingTab>(m_Parent, rf.path);
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
                ImGui::TextDisabled("Example Models");
                ImGui::Dummy({0.0f, 2.0f});

                for (std::filesystem::path const& ex : m_MainMenuFileTab.exampleOsimFiles)
                {
                    std::string const label = std::string{ICON_FA_FILE} + " " + ex.filename().string();

                    ImGui::PushID(++imguiID);
                    if (ImGui::MenuItem(label.c_str()))
                    {
                        m_Parent->selectTab(m_Parent->addTab<LoadingTab>(m_Parent, ex));
                    }
                    ImGui::PopID();
                }
            }
            ImGui::NextColumn();

            ImGui::Columns();
        }
        ImGui::End();
    }

    void drawAttributationLogos()
    {
        Rect const viewportRect = osc::GetMainViewportWorkspaceScreenRect();
        glm::vec2 loc = viewportRect.p2 - m_AttributationLogoDims - m_AttributationLogoPadding;

        ImGui::SetNextWindowPos(loc);
        ImGui::Begin("##tudlogo", nullptr, GetMinimalWindowFlags());
        osc::DrawTextureAsImGuiImage(m_TudLogo, m_AttributationLogoDims);
        ImGui::End();

        loc.x -= m_AttributationLogoDims.x + m_AttributationLogoPadding.x;

        ImGui::SetNextWindowPos(loc);
        ImGui::Begin("##czlogo", nullptr, GetMinimalWindowFlags());
        osc::DrawTextureAsImGuiImage(m_CziLogo, m_AttributationLogoDims);
        ImGui::End();
    }

    void drawVersionInfo()
    {
        Rect const tabRect = osc::GetMainViewportWorkspaceScreenRect();
        float const h = ImGui::GetTextLineHeightWithSpacing();
        float constexpr padding = 5.0f;

        glm::vec2 const pos
        {
            tabRect.p1.x + padding,
            tabRect.p2.y - h - padding,
        };

        ImDrawList* const dl = ImGui::GetForegroundDrawList();
        ImU32 const color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
        char const* const content = "OpenSim Creator v" OSC_VERSION_STRING " (build " OSC_BUILD_ID ")";
        dl->AddText(pos, color, content);
    }

    // tab data
    UID m_ID;
    std::string m_Name = ICON_FA_HOME;
    MainUIStateAPI* m_Parent;

    // for rendering the 3D scene
    osc::PolarPerspectiveCamera m_Camera = GetSplashScreenDefaultPolarCamera();
    SceneRenderer m_SceneRenderer{osc::App::config(), *osc::App::singleton<osc::MeshCache>(), *osc::App::singleton<osc::ShaderCache>()};
    SceneRendererParams m_LastSceneRendererParams = GetSplashScreenDefaultRenderParams(m_Camera);

    glm::vec2 m_MenuMaxDims = {640.0f, 512.0f};

    // main app logo, blitted to top of the screen
    Texture2D m_OscLogo = LoadTextureFromSVGResource("banner.svg");

    // attributation logos, blitted to bottom of screen
    Texture2D m_CziLogo = LoadTexture2DFromImage(App::resource("textures/chanzuckerberg_logo.png"), ImageFlags_FlipVertically);
    Texture2D m_TudLogo = LoadTexture2DFromImage(App::resource("textures/tud_logo.png"), ImageFlags_FlipVertically);

    // dimensions of stuff
    glm::vec2 m_TopLogoDims =  m_OscLogo.getDimensions();//[d =]() { return glm::vec2{d.x / (d.y/128.0f), 128.0f}; }();
    glm::vec2 m_TopLogoPadding = {25.0f, 35.0f};
    glm::vec2 m_AttributationLogoDims = {64.0f, 64.0f};
    glm::vec2 m_AttributationLogoPadding = {16.0f, 16.0f};

    // UI state
    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    LogViewer m_LogViewer;
};


// public API (PIMPL)

osc::SplashTab::SplashTab(MainUIStateAPI* parent) :
    m_Impl{std::make_unique<Impl>(std::move(parent))}
{
}

osc::SplashTab::SplashTab(SplashTab&&) noexcept = default;
osc::SplashTab& osc::SplashTab::operator=(SplashTab&&) noexcept = default;
osc::SplashTab::~SplashTab() noexcept = default;

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
