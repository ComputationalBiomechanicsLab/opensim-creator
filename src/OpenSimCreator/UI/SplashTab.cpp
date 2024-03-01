#include "SplashTab.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Platform/RecentFile.h>
#include <OpenSimCreator/Platform/RecentFiles.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/LoadingTab.h>
#include <OpenSimCreator/UI/FrameDefinition/FrameDefinitionTab.h>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterTab.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTab.h>
#include <OpenSimCreator/UI/Shared/MainMenu.h>

#include <IconsFontAwesome5.h>
#include <oscar/Formats/SVG.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneRenderer.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Graphics/Scene/ShaderCache.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Graphics/TextureFilterMode.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppConfig.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/UI/Widgets/LogViewer.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/ParentPtr.h>
#include <SDL_events.h>

#include <filesystem>
#include <span>
#include <string>
#include <utility>

using namespace osc::literals;
using namespace osc;

namespace
{
    PolarPerspectiveCamera GetSplashScreenDefaultPolarCamera()
    {
        PolarPerspectiveCamera rv;
        rv.phi = 30_deg;
        rv.radius = 10.0f;
        rv.theta = 45_deg;
        return rv;
    }

    SceneRendererParams GetSplashScreenDefaultRenderParams(PolarPerspectiveCamera const& camera)
    {
        SceneRendererParams rv;
        rv.drawRims = false;
        rv.viewMatrix = camera.view_matrix();
        rv.nearClippingPlane = camera.znear;
        rv.farClippingPlane = camera.zfar;
        rv.viewPos = camera.getPos();
        rv.lightDirection = {-0.34f, -0.25f, 0.05f};
        rv.lightColor = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f, 1.0f};
        rv.backgroundColor = {0.89f, 0.89f, 0.89f, 1.0f};
        return rv;
    }

    // helper: draws an ui::MenuItem for a given recent- or example-file-path
    void DrawRecentOrExampleFileMenuItem(
        std::filesystem::path const& path,
        ParentPtr<IMainUIStateAPI>& parent_,
        int& imguiID)
    {
        std::string const label = std::string{ICON_FA_FILE} + " " + path.filename().string();

        ui::PushID(++imguiID);
        if (ui::MenuItem(label.c_str()))
        {
            parent_->addAndSelectTab<LoadingTab>(parent_, path);
        }
        // show the full path as a tooltip when the item is hovered (some people have
        // long file names (#784)
        if (ui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ui::TextUnformatted(path.filename().string());
            ImGui::EndTooltip();
        }
        ui::PopID();
    }
}

class osc::SplashTab::Impl final {
public:

    explicit Impl(ParentPtr<IMainUIStateAPI> const& parent_) :
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
        if (e.type == SDL_DROPFILE && e.drop.file != nullptr && std::string_view{e.drop.file}.ends_with(".osim"))
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
        if (Area(GetMainViewportWorkspaceScreenRect()) <= 0.0f)
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
        Rect tabRect = GetMainViewportWorkspaceScreenRect();
        // pretend the attributation bar isn't there (avoid it)
        tabRect.p2.y -= static_cast<float>(std::max(m_TudLogo.getDimensions().y, m_CziLogo.getDimensions().y)) - 2.0f*ImGui::GetStyle().WindowPadding.y;

        Vec2 const menuAndTopLogoDims = elementwise_min(dimensions(tabRect), Vec2{m_SplashMenuMaxDims.x, m_SplashMenuMaxDims.y + m_MainAppLogoDims.y + m_TopLogoPadding.y});
        Vec2 const menuAndTopLogoTopLeft = tabRect.p1 + 0.5f*(dimensions(tabRect) - menuAndTopLogoDims);
        Vec2 const menuDims = {menuAndTopLogoDims.x, menuAndTopLogoDims.y - m_MainAppLogoDims.y - m_TopLogoPadding.y};
        Vec2 const menuTopLeft = Vec2{menuAndTopLogoTopLeft.x, menuAndTopLogoTopLeft.y + m_MainAppLogoDims.y + m_TopLogoPadding.y};

        return Rect{menuTopLeft, menuTopLeft + menuDims};
    }

    Rect calcLogoRect() const
    {
        Rect const mmr = calcMainMenuRect();
        Vec2 const topLeft
        {
            mmr.p1.x + dimensions(mmr).x/2.0f - m_MainAppLogoDims.x/2.0f,
            mmr.p1.y - m_TopLogoPadding.y - m_MainAppLogoDims.y,
        };

        return Rect{topLeft, topLeft + m_MainAppLogoDims};
    }

    void drawBackground()
    {
        Rect const screenRect = GetMainViewportWorkspaceScreenRect();

        ImGui::SetNextWindowPos(screenRect.p1);
        ImGui::SetNextWindowSize(dimensions(screenRect));

        ui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
        ui::Begin("##splashscreenbackground", nullptr, GetMinimalWindowFlags());
        ui::PopStyleVar();

        SceneRendererParams params{m_LastSceneRendererParams};
        params.dimensions = dimensions(screenRect);
        params.antiAliasingLevel = App::get().getCurrentAntiAliasingLevel();
        params.projectionMatrix = m_Camera.projection_matrix(AspectRatio(screenRect));

        if (params != m_LastSceneRendererParams)
        {
            m_SceneRenderer.render({}, params);
            m_LastSceneRendererParams = params;
        }

        DrawTextureAsImGuiImage(m_SceneRenderer.updRenderTexture());

        ui::End();
    }

    void drawLogo()
    {
        Rect const logoRect = calcLogoRect();

        ImGui::SetNextWindowPos(logoRect.p1);
        ui::Begin("##osclogo", nullptr, GetMinimalWindowFlags());
        DrawTextureAsImGuiImage(m_MainAppLogo, dimensions(logoRect));
        ui::End();
    }

    void drawMenu()
    {
        // center the menu window
        Rect const mmr = calcMainMenuRect();
        ImGui::SetNextWindowPos(mmr.p1);
        ImGui::SetNextWindowSize({dimensions(mmr).x, -1.0f});
        ImGui::SetNextWindowSizeConstraints(dimensions(mmr), dimensions(mmr));

        if (ui::Begin("Splash screen", nullptr, ImGuiWindowFlags_NoTitleBar))
        {
            drawMenuContent();
        }
        ui::End();
    }

    void drawMenuContent()
    {
        // de-dupe imgui IDs because these lists may contain duplicate
        // names
        int imguiID = 0;

        ui::Columns(2, nullptr, false);
        drawMenuLeftColumnContent(imguiID);
        ui::NextColumn();
        drawMenuRightColumnContent(imguiID);
        ui::NextColumn();
        ui::Columns();
    }

    void drawActionsMenuSectionContent()
    {
        if (ui::MenuItem(ICON_FA_FILE " New Model"))
        {
            ActionNewModel(m_Parent);
        }
        if (ui::MenuItem(ICON_FA_FOLDER_OPEN " Open Model"))
        {
            ActionOpenModel(m_Parent);
        }
        if (ui::MenuItem(ICON_FA_MAGIC " Import Meshes"))
        {
            m_Parent->addAndSelectTab<mi::MeshImporterTab>(m_Parent);
        }
        App::upd().addFrameAnnotation("SplashTab/ImportMeshesMenuItem", GetItemRect());
        if (ui::MenuItem(ICON_FA_BOOK " Open Documentation"))
        {
            OpenPathInOSDefaultApplication(App::get().getConfig().getHTMLDocsDir() / "index.html");
        }
    }

    void drawWorkflowsMenuSectionContent()
    {
        if (ui::MenuItem(ICON_FA_ARROWS_ALT " Frame Definition"))
        {
            m_Parent->addAndSelectTab<FrameDefinitionTab>(m_Parent);
        }
        if (ui::MenuItem(ICON_FA_MAGIC " Mesh Importer"))
        {
            m_Parent->addAndSelectTab<mi::MeshImporterTab>(m_Parent);
        }
        if (ui::MenuItem(ICON_FA_CUBE " Mesh Warping"))
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
            PushStyleColor(ImGuiCol_Text, Color::halfGrey());
            ui::TextWrapped("No files opened recently. Try:");
            ImGui::BulletText("Creating a new model (Ctrl+N)");
            ImGui::BulletText("Opening an existing model (Ctrl+O)");
            ImGui::BulletText("Opening an example (right-side)");
            PopStyleColor();
        }
    }

    void drawMenuLeftColumnContent(int& imguiID)
    {
        ui::TextDisabled("Actions");
        ui::Dummy({0.0f, 2.0f});

        drawActionsMenuSectionContent();

        ui::Dummy({0.0f, 1.0f*ImGui::GetTextLineHeight()});
        ui::TextDisabled("Workflows");
        ui::Dummy({0.0f, 2.0f});

        drawWorkflowsMenuSectionContent();

        ui::Dummy({0.0f, 1.0f*ImGui::GetTextLineHeight()});
        ui::TextDisabled("Recent Models");
        ui::Dummy({0.0f, 2.0f});

        drawRecentlyOpenedFilesMenuSectionContent(imguiID);
    }

    void drawMenuRightColumnContent(int& imguiID)
    {
        if (!m_MainMenuFileTab.exampleOsimFiles.empty())
        {
            ui::TextDisabled("Example Models");
            ui::Dummy({0.0f, 2.0f});

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
        ui::Begin("##czlogo", nullptr, GetMinimalWindowFlags());
        DrawTextureAsImGuiImage(m_CziLogo);
        ui::End();

        loc.x += static_cast<float>(m_CziLogo.getDimensions().x) + 2.0f*ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetNextWindowPos(loc);
        ui::Begin("##tudlogo", nullptr, GetMinimalWindowFlags());
        DrawTextureAsImGuiImage(m_TudLogo);
        ui::End();
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
        ImU32 const color = ToImU32(Color::black());
        std::string const text = CalcFullApplicationNameWithVersionAndBuild(App::get().getMetadata());
        dl->AddText(pos, color, text.c_str());
    }

    // tab data
    UID m_TabID;
    ParentPtr<IMainUIStateAPI> m_Parent;

    // for rendering the 3D scene
    PolarPerspectiveCamera m_Camera = GetSplashScreenDefaultPolarCamera();
    SceneRenderer m_SceneRenderer
    {
        *App::singleton<SceneCache>(),
        *App::singleton<ShaderCache>(App::resource_loader()),
    };
    SceneRendererParams m_LastSceneRendererParams = GetSplashScreenDefaultRenderParams(m_Camera);

    Texture2D m_MainAppLogo = ReadSVGIntoTexture(App::load_resource("textures/banner.svg"));
    Texture2D m_CziLogo = ReadSVGIntoTexture(App::load_resource("textures/chanzuckerberg_logo.svg"), 0.5f);
    Texture2D m_TudLogo = ReadSVGIntoTexture(App::load_resource("textures/tudelft_logo.svg"), 0.5f);

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

osc::SplashTab::SplashTab(ParentPtr<IMainUIStateAPI> const& parent_) :
    m_Impl{std::make_unique<Impl>(parent_)}
{
}

osc::SplashTab::SplashTab(SplashTab&&) noexcept = default;
osc::SplashTab& osc::SplashTab::operator=(SplashTab&&) noexcept = default;
osc::SplashTab::~SplashTab() noexcept = default;

UID osc::SplashTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::SplashTab::implGetName() const
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
