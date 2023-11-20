#include "SplashTab.hpp"

#include <OpenSimCreator/Platform/RecentFile.hpp>
#include <OpenSimCreator/Platform/RecentFiles.hpp>
#include <OpenSimCreator/UI/Middleware/MainUIStateAPI.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/MeshImporterTab.hpp>
#include <OpenSimCreator/UI/Tabs/FrameDefinitionTab.hpp>
#include <OpenSimCreator/UI/Tabs/LoadingTab.hpp>
#include <OpenSimCreator/UI/Tabs/MeshWarpingTab.hpp>
#include <OpenSimCreator/UI/Widgets/MainMenu.hpp>
#include <OpenSimCreator/Utils/UndoableModelActions.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Formats/SVG.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Graphics/TextureFilterMode.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/Platform/AppMetadata.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/SceneRenderer.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>
#include <oscar/UI/Tabs/TabHost.hpp>
#include <oscar/UI/Widgets/LogViewer.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <SDL_events.h>

#include <filesystem>
#include <numbers>
#include <span>
#include <string>
#include <utility>

namespace
{
    osc::PolarPerspectiveCamera GetSplashScreenDefaultPolarCamera()
    {
        osc::PolarPerspectiveCamera rv;
        rv.phi = std::numbers::pi_v<float>/6.0f;
        rv.radius = 10.0f;
        rv.theta = std::numbers::pi_v<float>/4.0f;
        return rv;
    }

    osc::SceneRendererParams GetSplashScreenDefaultRenderParams(osc::PolarPerspectiveCamera const& camera)
    {
        osc::SceneRendererParams rv;
        rv.drawRims = false;
        rv.viewMatrix = camera.getViewMtx();
        rv.nearClippingPlane = camera.znear;
        rv.farClippingPlane = camera.zfar;
        rv.viewPos = camera.getPos();
        rv.lightDirection = {-0.34f, -0.25f, 0.05f};
        rv.lightColor = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f, 1.0f};
        rv.backgroundColor = {0.89f, 0.89f, 0.89f, 1.0f};
        return rv;
    }

    // helper: draws an ImGui::MenuItem for a given recent- or example-file-path
    void DrawRecentOrExampleFileMenuItem(
        std::filesystem::path const& path,
        osc::ParentPtr<osc::MainUIStateAPI>& parent_,
        int& imguiID)
    {
        std::string const label = std::string{ICON_FA_FILE} + " " + path.filename().string();

        ImGui::PushID(++imguiID);
        if (ImGui::MenuItem(label.c_str()))
        {
            parent_->addAndSelectTab<osc::LoadingTab>(parent_, path);
        }
        // show the full path as a tooltip when the item is hovered (some people have
        // long file names (#784)
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(path.filename().string().c_str());
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    }
}

class osc::SplashTab::Impl final {
public:

    explicit Impl(ParentPtr<MainUIStateAPI> const& parent_) :
        m_Parent{parent_}
    {
        m_MainAppLogo.setFilterMode(TextureFilterMode::Linear);
        m_CziLogo.setFilterMode(TextureFilterMode::Linear);
        m_TudLogo.setFilterMode(TextureFilterMode::Linear);
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_HOME;
    }

    void onMount()
    {
        // edge-case: reset the file tab whenever the splash screen is (re)mounted,
        // because actions within other tabs may have updated things like recently
        // used files etc. (#618)
        m_MainMenuFileTab = MainMenuFileTab{};

        App::upd().makeMainEventLoopWaiting();
    }

    void onUnmount()
    {
        App::upd().makeMainEventLoopPolling();
    }

    bool onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_DROPFILE && e.drop.file != nullptr && EndsWith(e.drop.file, ".osim"))
        {
            // if the user drops an osim file on this tab then it should be loaded
            m_Parent->addAndSelectTab<LoadingTab>(m_Parent, e.drop.file);
            return true;
        }
        return false;
    }

    void drawMainMenu()
    {
        m_MainMenuFileTab.onDraw(m_Parent);
        m_MainMenuAboutTab.onDraw();
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
        // pretend the attributation bar isn't there (avoid it)
        tabRect.p2.y -= static_cast<float>(std::max(m_TudLogo.getDimensions().y, m_CziLogo.getDimensions().y)) - 2.0f*ImGui::GetStyle().WindowPadding.y;

        Vec2 const menuAndTopLogoDims = osc::Min(osc::Dimensions(tabRect), Vec2{m_SplashMenuMaxDims.x, m_SplashMenuMaxDims.y + m_MainAppLogoDims.y + m_TopLogoPadding.y});
        Vec2 const menuAndTopLogoTopLeft = tabRect.p1 + 0.5f*(Dimensions(tabRect) - menuAndTopLogoDims);
        Vec2 const menuDims = {menuAndTopLogoDims.x, menuAndTopLogoDims.y - m_MainAppLogoDims.y - m_TopLogoPadding.y};
        Vec2 const menuTopLeft = Vec2{menuAndTopLogoTopLeft.x, menuAndTopLogoTopLeft.y + m_MainAppLogoDims.y + m_TopLogoPadding.y};

        return Rect{menuTopLeft, menuTopLeft + menuDims};
    }

    Rect calcLogoRect() const
    {
        Rect const mmr = calcMainMenuRect();
        Vec2 const topLeft
        {
            mmr.p1.x + Dimensions(mmr).x/2.0f - m_MainAppLogoDims.x/2.0f,
            mmr.p1.y - m_TopLogoPadding.y - m_MainAppLogoDims.y,
        };

        return Rect{topLeft, topLeft + m_MainAppLogoDims};
    }

    void drawBackground()
    {
        Rect const screenRect = GetMainViewportWorkspaceScreenRect();

        ImGui::SetNextWindowPos(screenRect.p1);
        ImGui::SetNextWindowSize(Dimensions(screenRect));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
        ImGui::Begin("##splashscreenbackground", nullptr, GetMinimalWindowFlags());
        ImGui::PopStyleVar();

        SceneRendererParams params{m_LastSceneRendererParams};
        params.dimensions = Dimensions(screenRect);
        params.antiAliasingLevel = App::get().getCurrentAntiAliasingLevel();
        params.projectionMatrix = m_Camera.getProjMtx(AspectRatio(screenRect));

        if (params != m_LastSceneRendererParams)
        {
            m_SceneRenderer.render({}, params);
            m_LastSceneRendererParams = params;
        }

        DrawTextureAsImGuiImage(m_SceneRenderer.updRenderTexture());

        ImGui::End();
    }

    void drawLogo()
    {
        Rect const logoRect = calcLogoRect();

        ImGui::SetNextWindowPos(logoRect.p1);
        ImGui::Begin("##osclogo", nullptr, GetMinimalWindowFlags());
        DrawTextureAsImGuiImage(m_MainAppLogo, Dimensions(logoRect));
        ImGui::End();
    }

    void drawMenu()
    {
        // center the menu window
        Rect const mmr = calcMainMenuRect();
        ImGui::SetNextWindowPos(mmr.p1);
        ImGui::SetNextWindowSize({Dimensions(mmr).x, -1.0f});
        ImGui::SetNextWindowSizeConstraints(Dimensions(mmr), Dimensions(mmr));

        if (ImGui::Begin("Splash screen", nullptr, ImGuiWindowFlags_NoTitleBar))
        {
            drawMenuContent();
        }
        ImGui::End();
    }

    void drawMenuContent()
    {
        // de-dupe imgui IDs because these lists may contain duplicate
        // names
        int imguiID = 0;

        ImGui::Columns(2, nullptr, false);
        drawMenuLeftColumnContent(imguiID);
        ImGui::NextColumn();
        drawMenuRightColumnContent(imguiID);
        ImGui::NextColumn();
        ImGui::Columns();
    }

    void drawActionsMenuSectionContent()
    {
        if (ImGui::MenuItem(ICON_FA_FILE " New Model"))
        {
            ActionNewModel(m_Parent);
        }
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Model"))
        {
            ActionOpenModel(m_Parent);
        }
        if (ImGui::MenuItem(ICON_FA_MAGIC " Import Meshes"))
        {
            m_Parent->addAndSelectTab<MeshImporterTab>(m_Parent);
        }
        App::upd().addFrameAnnotation("SplashTab/ImportMeshesMenuItem", GetItemRect());
        if (ImGui::MenuItem(ICON_FA_BOOK " Open Documentation"))
        {
            OpenPathInOSDefaultApplication(App::get().getConfig().getHTMLDocsDir() / "index.html");
        }
    }

    void drawWorkflowsMenuSectionContent()
    {
        if (ImGui::MenuItem(ICON_FA_ARROWS_ALT " Frame Definition"))
        {
            m_Parent->addAndSelectTab<FrameDefinitionTab>(m_Parent);
        }
        if (ImGui::MenuItem(ICON_FA_MAGIC " Mesh Importer"))
        {
            m_Parent->addAndSelectTab<MeshImporterTab>(m_Parent);
        }
        if (ImGui::MenuItem(ICON_FA_CUBE " Mesh Warping"))
        {
            m_Parent->addAndSelectTab<MeshWarpingTab>(m_Parent);
        }
    }

    void drawRecentlyOpenedFilesMenuSectionContent(int& imguiID)
    {
        auto const recentFiles = App::singleton<RecentFiles>();
        if (!recentFiles->empty())
        {
            for (RecentFile const& rf : *recentFiles)
            {
                DrawRecentOrExampleFileMenuItem(
                    rf.path,
                    m_Parent,
                    imguiID
                );
            }
        }
        else
        {
            osc::PushStyleColor(ImGuiCol_Text, osc::Color::halfGrey());
            ImGui::TextWrapped("No files opened recently. Try:");
            ImGui::BulletText("Creating a new model (Ctrl+N)");
            ImGui::BulletText("Opening an existing model (Ctrl+O)");
            ImGui::BulletText("Opening an example (right-side)");
            osc::PopStyleColor();
        }
    }

    void drawMenuLeftColumnContent(int& imguiID)
    {
        ImGui::TextDisabled("Actions");
        ImGui::Dummy({0.0f, 2.0f});

        drawActionsMenuSectionContent();

        ImGui::Dummy({0.0f, 1.0f*ImGui::GetTextLineHeight()});
        ImGui::TextDisabled("Workflows");
        ImGui::Dummy({0.0f, 2.0f});

        drawWorkflowsMenuSectionContent();

        ImGui::Dummy({0.0f, 1.0f*ImGui::GetTextLineHeight()});
        ImGui::TextDisabled("Recent Models");
        ImGui::Dummy({0.0f, 2.0f});

        drawRecentlyOpenedFilesMenuSectionContent(imguiID);
    }

    void drawMenuRightColumnContent(int& imguiID)
    {
        if (!m_MainMenuFileTab.exampleOsimFiles.empty())
        {
            ImGui::TextDisabled("Example Models");
            ImGui::Dummy({0.0f, 2.0f});

            for (std::filesystem::path const& examplePath : m_MainMenuFileTab.exampleOsimFiles)
            {
                DrawRecentOrExampleFileMenuItem(
                    examplePath,
                    m_Parent,
                    imguiID
                );
            }
        }
    }

    void drawAttributationLogos()
    {
        Rect const viewportRect = GetMainViewportWorkspaceScreenRect();
        Vec2 loc = viewportRect.p2;
        loc.x = loc.x - 2.0f*ImGui::GetStyle().WindowPadding.x - static_cast<float>(m_CziLogo.getDimensions().x) - 2.0f*ImGui::GetStyle().ItemSpacing.x - static_cast<float>(m_TudLogo.getDimensions().x);
        loc.y = loc.y - 2.0f*ImGui::GetStyle().WindowPadding.y - static_cast<float>(std::max(m_CziLogo.getDimensions().y, m_TudLogo.getDimensions().y));

        ImGui::SetNextWindowPos(loc);
        ImGui::Begin("##czlogo", nullptr, GetMinimalWindowFlags());
        DrawTextureAsImGuiImage(m_CziLogo);
        ImGui::End();

        loc.x += static_cast<float>(m_CziLogo.getDimensions().x) + 2.0f*ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetNextWindowPos(loc);
        ImGui::Begin("##tudlogo", nullptr, GetMinimalWindowFlags());
        DrawTextureAsImGuiImage(m_TudLogo);
        ImGui::End();
    }

    void drawVersionInfo()
    {
        Rect const tabRect = GetMainViewportWorkspaceScreenRect();
        float const h = ImGui::GetTextLineHeightWithSpacing();
        float const padding = 5.0f;

        Vec2 const pos
        {
            tabRect.p1.x + padding,
            tabRect.p2.y - h - padding,
        };

        ImDrawList* const dl = ImGui::GetForegroundDrawList();
        ImU32 const color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
        std::string const text = CalcFullApplicationNameWithVersionAndBuild(App::get().getMetadata());
        dl->AddText(pos, color, text.c_str());
    }

    // tab data
    UID m_TabID;
    ParentPtr<MainUIStateAPI> m_Parent;

    // for rendering the 3D scene
    PolarPerspectiveCamera m_Camera = GetSplashScreenDefaultPolarCamera();
    SceneRenderer m_SceneRenderer
    {
        App::config(),
        *App::singleton<SceneCache>(),
        *App::singleton<ShaderCache>(),
    };
    SceneRendererParams m_LastSceneRendererParams = GetSplashScreenDefaultRenderParams(m_Camera);

    Texture2D m_MainAppLogo = LoadTextureFromSVGFile(App::resource("textures/banner.svg"));
    Texture2D m_CziLogo = LoadTextureFromSVGFile(App::resource("textures/chanzuckerberg_logo.svg"), 0.5f);
    Texture2D m_TudLogo = LoadTextureFromSVGFile(App::resource("textures/tudelft_logo.svg"), 0.5f);

    // dimensions of stuff
    Vec2 m_SplashMenuMaxDims = {640.0f, 512.0f};
    Vec2 m_MainAppLogoDims =  m_MainAppLogo.getDimensions();
    Vec2 m_TopLogoPadding = {25.0f, 35.0f};

    // UI state
    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    LogViewer m_LogViewer;
};


// public API (PIMPL)

osc::SplashTab::SplashTab(ParentPtr<MainUIStateAPI> const& parent_) :
    m_Impl{std::make_unique<Impl>(parent_)}
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

void osc::SplashTab::implOnDrawMainMenu()
{
    m_Impl->drawMainMenu();
}

void osc::SplashTab::implOnDraw()
{
    m_Impl->onDraw();
}
