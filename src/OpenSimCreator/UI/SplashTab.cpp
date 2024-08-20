#include "SplashTab.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Platform/OpenSimCreatorApp.h>
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
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Graphics/TextureFilterMode.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Platform/AppSettings.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/UI/Widgets/LogViewer.h>
#include <oscar/Utils/Algorithms.h>
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

    SceneRendererParams GetSplashScreenDefaultRenderParams(const PolarPerspectiveCamera& camera)
    {
        SceneRendererParams rv;
        rv.draw_rims = false;
        rv.view_matrix = camera.view_matrix();
        rv.near_clipping_plane = camera.znear;
        rv.far_clipping_plane = camera.zfar;
        rv.view_pos = camera.position();
        rv.light_direction = {-0.34f, -0.25f, 0.05f};
        rv.light_color = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f, 1.0f};
        rv.background_color = {0.89f, 0.89f, 0.89f, 1.0f};
        return rv;
    }

    // helper: draws an ui::draw_menu_item for a given recent- or example-file-path
    void DrawRecentOrExampleFileMenuItem(
        const std::filesystem::path& path,
        ParentPtr<IMainUIStateAPI>& parent_,
        int& imguiID)
    {
        const std::string label = std::string{ICON_FA_FILE} + " " + path.filename().string();

        ui::push_id(++imguiID);
        if (ui::draw_menu_item(label)) {
            parent_->add_and_select_tab<LoadingTab>(parent_, path);
        }
        // show the full path as a tooltip when the item is hovered (some people have
        // long file names (#784)
        if (ui::is_item_hovered()) {
            ui::begin_tooltip_nowrap();
            ui::draw_text_unformatted(path.filename().string());
            ui::end_tooltip_nowrap();
        }
        ui::pop_id();
    }
}

class osc::SplashTab::Impl final {
public:

    explicit Impl(const ParentPtr<IMainUIStateAPI>& parent_) :
        m_Parent{parent_}
    {
        m_MainAppLogo.set_filter_mode(TextureFilterMode::Linear);
        m_CziLogo.set_filter_mode(TextureFilterMode::Linear);
        m_TudLogo.set_filter_mode(TextureFilterMode::Linear);
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_HOME;
    }

    void on_mount()
    {
        // edge-case: reset the file tab whenever the splash screen is (re)mounted,
        // because actions within other tabs may have updated things like recently
        // used files etc. (#618)
        m_MainMenuFileTab = MainMenuFileTab{};

        App::upd().make_main_loop_waiting();
    }

    void on_unmount()
    {
        App::upd().make_main_loop_polling();
    }

    bool onEvent(const SDL_Event& e)
    {
        if (e.type == SDL_DROPFILE && e.drop.file != nullptr && std::string_view{e.drop.file}.ends_with(".osim")) {
            // if the user drops an osim file on this tab then it should be loaded
            m_Parent->add_and_select_tab<LoadingTab>(m_Parent, e.drop.file);
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
        if (area_of(ui::get_main_viewport_workspace_uiscreenspace_rect()) <= 0.0f) {
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
        Rect tabUIRect = ui::get_main_viewport_workspace_uiscreenspace_rect();
        // pretend the attributation bar isn't there (avoid it)
        tabUIRect.p2.y -= static_cast<float>(max(m_TudLogo.dimensions().y, m_CziLogo.dimensions().y)) - 2.0f*ui::get_style_panel_padding().y;

        const Vec2 menuAndTopLogoDims = elementwise_min(dimensions_of(tabUIRect), Vec2{m_SplashMenuMaxDims.x, m_SplashMenuMaxDims.y + m_MainAppLogoDims.y + m_TopLogoPadding.y});
        const Vec2 menuAndTopLogoTopLeft = tabUIRect.p1 + 0.5f*(dimensions_of(tabUIRect) - menuAndTopLogoDims);
        const Vec2 menuDims = {menuAndTopLogoDims.x, menuAndTopLogoDims.y - m_MainAppLogoDims.y - m_TopLogoPadding.y};
        const Vec2 menuTopLeft = Vec2{menuAndTopLogoTopLeft.x, menuAndTopLogoTopLeft.y + m_MainAppLogoDims.y + m_TopLogoPadding.y};

        return Rect{menuTopLeft, menuTopLeft + menuDims};
    }

    Rect calcLogoRect() const
    {
        const Rect mmr = calcMainMenuRect();
        const Vec2 topLeft{
            mmr.p1.x + dimensions_of(mmr).x/2.0f - m_MainAppLogoDims.x/2.0f,
            mmr.p1.y - m_TopLogoPadding.y - m_MainAppLogoDims.y,
        };

        return Rect{topLeft, topLeft + m_MainAppLogoDims};
    }

    void drawBackground()
    {
        const Rect viewportUIRect = ui::get_main_viewport_workspace_uiscreenspace_rect();

        ui::set_next_panel_pos(viewportUIRect.p1);
        ui::set_next_panel_size(dimensions_of(viewportUIRect));

        ui::push_style_var(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
        ui::begin_panel("##splashscreenbackground", nullptr, ui::get_minimal_panel_flags());
        ui::pop_style_var();

        SceneRendererParams params{m_LastSceneRendererParams};
        params.dimensions = dimensions_of(viewportUIRect);
        params.antialiasing_level = App::get().anti_aliasing_level();
        params.projection_matrix = m_Camera.projection_matrix(aspect_ratio_of(viewportUIRect));

        if (params != m_LastSceneRendererParams) {
            scene_renderer_.render({}, params);
            m_LastSceneRendererParams = params;
        }

        ui::draw_image(scene_renderer_.upd_render_texture());

        ui::end_panel();
    }

    void drawLogo()
    {
        const Rect logoRect = calcLogoRect();

        ui::set_next_panel_pos(logoRect.p1);
        ui::begin_panel("##osclogo", nullptr, ui::get_minimal_panel_flags());
        ui::draw_image(m_MainAppLogo, dimensions_of(logoRect));
        ui::end_panel();
    }

    void drawMenu()
    {
        // center the menu window
        const Rect mmr = calcMainMenuRect();
        ui::set_next_panel_pos(mmr.p1);
        ui::set_next_panel_size({dimensions_of(mmr).x, -1.0f});
        ui::set_next_panel_size_constraints(dimensions_of(mmr), dimensions_of(mmr));

        if (ui::begin_panel("Splash screen", nullptr, ImGuiWindowFlags_NoTitleBar)) {
            drawMenuContent();
        }
        ui::end_panel();
    }

    void drawMenuContent()
    {
        // de-dupe imgui IDs because these lists may contain duplicate
        // names
        int imguiID = 0;

        ui::set_num_columns(2, std::nullopt, false);
        drawMenuLeftColumnContent(imguiID);
        ui::next_column();
        drawMenuRightColumnContent(imguiID);
        ui::next_column();
        ui::set_num_columns();
    }

    void drawActionsMenuSectionContent()
    {
        if (ui::draw_menu_item(ICON_FA_FILE " New Model")) {
            ActionNewModel(m_Parent);
        }
        if (ui::draw_menu_item(ICON_FA_FOLDER_OPEN " Open Model")) {
            ActionOpenModel(m_Parent);
        }
        if (ui::draw_menu_item(ICON_FA_MAGIC " Import Meshes")) {
            m_Parent->add_and_select_tab<mi::MeshImporterTab>(m_Parent);
        }
        App::upd().add_frame_annotation("SplashTab/ImportMeshesMenuItem", ui::get_last_drawn_item_screen_rect());
        if (ui::draw_menu_item(ICON_FA_BOOK " Open Documentation")) {
            open_url_in_os_default_web_browser(OpenSimCreatorApp::get().docs_url());
        }
    }

    void drawWorkflowsMenuSectionContent()
    {
        if (ui::draw_menu_item(ICON_FA_ARROWS_ALT " Frame Definition")) {
            m_Parent->add_and_select_tab<FrameDefinitionTab>(m_Parent);
        }
        if (ui::draw_menu_item(ICON_FA_MAGIC " Mesh Importer")) {
            m_Parent->add_and_select_tab<mi::MeshImporterTab>(m_Parent);
        }
        if (ui::draw_menu_item(ICON_FA_CUBE " Mesh Warping")) {
            m_Parent->add_and_select_tab<MeshWarpingTab>(m_Parent);
        }
        App::upd().add_frame_annotation("SplashTab/MeshWarpingMenuItem", ui::get_last_drawn_item_screen_rect());
    }

    void drawRecentlyOpenedFilesMenuSectionContent(int& imguiID)
    {
        const auto recentFiles = App::singleton<RecentFiles>();
        if (!recentFiles->empty()) {
            for (const RecentFile& rf : *recentFiles) {
                DrawRecentOrExampleFileMenuItem(
                    rf.path,
                    m_Parent,
                    imguiID
                );
            }
        }
        else {
            ui::push_style_color(ImGuiCol_Text, Color::half_grey());
            ui::draw_text_wrapped("No files opened recently. Try:");
            ui::draw_text_bullet_pointed("Creating a new model (Ctrl+N)");
            ui::draw_text_bullet_pointed("Opening an existing model (Ctrl+O)");
            ui::draw_text_bullet_pointed("Opening an example (right-side)");
            ui::pop_style_color();
        }
    }

    void drawMenuLeftColumnContent(int& imguiID)
    {
        ui::draw_text_disabled("Actions");
        ui::draw_dummy({0.0f, 2.0f});

        drawActionsMenuSectionContent();

        ui::draw_dummy({0.0f, 1.0f*ui::get_text_line_height()});
        ui::draw_text_disabled("Workflows");
        ui::draw_dummy({0.0f, 2.0f});

        drawWorkflowsMenuSectionContent();

        ui::draw_dummy({0.0f, 1.0f*ui::get_text_line_height()});
        ui::draw_text_disabled("Recent Models");
        ui::draw_dummy({0.0f, 2.0f});

        drawRecentlyOpenedFilesMenuSectionContent(imguiID);
    }

    void drawMenuRightColumnContent(int& imguiID)
    {
        if (!m_MainMenuFileTab.exampleOsimFiles.empty()) {
            ui::draw_text_disabled("Example Models");
            ui::draw_dummy({0.0f, 2.0f});

            for (const std::filesystem::path& examplePath : m_MainMenuFileTab.exampleOsimFiles) {
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
        const Rect viewportUIRect = ui::get_main_viewport_workspace_uiscreenspace_rect();
        Vec2 loc = viewportUIRect.p2;
        loc.x = loc.x - 2.0f*ui::get_style_panel_padding().x - static_cast<float>(m_CziLogo.dimensions().x) - 2.0f*ui::get_style_item_spacing().x - static_cast<float>(m_TudLogo.dimensions().x);
        loc.y = loc.y - 2.0f*ui::get_style_panel_padding().y - static_cast<float>(max(m_CziLogo.dimensions().y, m_TudLogo.dimensions().y));

        ui::set_next_panel_pos(loc);
        ui::begin_panel("##czlogo", nullptr, ui::get_minimal_panel_flags());
        ui::draw_image(m_CziLogo);
        ui::end_panel();

        loc.x += static_cast<float>(m_CziLogo.dimensions().x) + 2.0f*ui::get_style_item_spacing().x;
        ui::set_next_panel_pos(loc);
        ui::begin_panel("##tudlogo", nullptr, ui::get_minimal_panel_flags());
        ui::draw_image(m_TudLogo);
        ui::end_panel();
    }

    void drawVersionInfo()
    {
        const Rect tabUIRect = ui::get_main_viewport_workspace_uiscreenspace_rect();
        const float h = ui::get_text_line_height_with_spacing();
        const float padding = 5.0f;

        const Vec2 pos{
            tabUIRect.p1.x + padding,
            tabUIRect.p2.y - h - padding,
        };

        ImDrawList* const dl = ui::get_foreground_draw_list();
        const ImU32 color = ui::to_ImU32(Color::black());
        const std::string text = calc_full_application_name_with_version_and_build_id(App::get().metadata());
        dl->AddText(pos, color, text.c_str());
    }

    // tab data
    UID m_TabID;
    ParentPtr<IMainUIStateAPI> m_Parent;

    // for rendering the 3D scene
    PolarPerspectiveCamera m_Camera = GetSplashScreenDefaultPolarCamera();
    SceneRenderer scene_renderer_{
        *App::singleton<SceneCache>(App::resource_loader()),
    };
    SceneRendererParams m_LastSceneRendererParams = GetSplashScreenDefaultRenderParams(m_Camera);

    Texture2D m_MainAppLogo = load_texture2D_from_svg(App::load_resource("textures/banner.svg"));
    Texture2D m_CziLogo = load_texture2D_from_svg(App::load_resource("textures/chanzuckerberg_logo.svg"), 0.5f);
    Texture2D m_TudLogo = load_texture2D_from_svg(App::load_resource("textures/tudelft_logo.svg"), 0.5f);

    // dimensions of stuff
    Vec2 m_SplashMenuMaxDims = {640.0f, 512.0f};
    Vec2 m_MainAppLogoDims =  m_MainAppLogo.dimensions();
    Vec2 m_TopLogoPadding = {25.0f, 35.0f};

    // UI state
    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    LogViewer m_LogViewer;
};


// public API (PIMPL)

osc::SplashTab::SplashTab(const ParentPtr<IMainUIStateAPI>& parent_) :
    m_Impl{std::make_unique<Impl>(parent_)}
{}

osc::SplashTab::SplashTab(SplashTab&&) noexcept = default;
osc::SplashTab& osc::SplashTab::operator=(SplashTab&&) noexcept = default;
osc::SplashTab::~SplashTab() noexcept = default;

UID osc::SplashTab::impl_get_id() const
{
    return m_Impl->getID();
}

CStringView osc::SplashTab::impl_get_name() const
{
    return m_Impl->getName();
}

void osc::SplashTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::SplashTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::SplashTab::impl_on_event(const SDL_Event& e)
{
    return m_Impl->onEvent(e);
}

void osc::SplashTab::impl_on_draw_main_menu()
{
    m_Impl->drawMainMenu();
}

void osc::SplashTab::impl_on_draw()
{
    m_Impl->onDraw();
}
