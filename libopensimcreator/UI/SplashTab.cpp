#include "SplashTab.h"

#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Platform/OpenSimCreatorApp.h>
#include <libopensimcreator/Platform/RecentFile.h>
#include <libopensimcreator/Platform/RecentFiles.h>
#include <libopensimcreator/UI/FrameDefinition/FrameDefinitionTab.h>
#include <libopensimcreator/UI/LoadingTab.h>
#include <libopensimcreator/UI/MeshImporter/MeshImporterTab.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTab.h>
#include <libopensimcreator/UI/ModelWarper/ModelWarperTab.h>
#include <libopensimcreator/UI/PreviewExperimentalData/PreviewExperimentalDataTab.h>
#include <libopensimcreator/UI/Shared/MainMenu.h>

#include <liboscar/Formats/SVG.h>
#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneRenderer.h>
#include <liboscar/Graphics/Scene/SceneRendererParams.h>
#include <liboscar/Graphics/Texture2D.h>
#include <liboscar/Graphics/TextureFilterMode.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/AppMetadata.h>
#include <liboscar/Platform/AppSettings.h>
#include <liboscar/Platform/Events/DropFileEvent.h>
#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Platform/os.h>
#include <liboscar/UI/Events/OpenTabEvent.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Tabs/TabPrivate.h>
#include <liboscar/UI/Widgets/LogViewer.h>
#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/LifetimedPtr.h>

#include <filesystem>
#include <memory>
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
        Widget& parent_,
        int& imguiID)
    {
        const std::string label = std::string{OSC_ICON_FILE} + " " + path.filename().string();

        ui::push_id(++imguiID);
        if (ui::draw_menu_item(label)) {
            auto tab = std::make_unique<LoadingTab>(parent_, path);
            App::post_event<OpenTabEvent>(parent_, std::move(tab));
        }
        // show the full path as a tooltip when the item is hovered (some people have
        // long file names (#784)
        if (ui::is_item_hovered()) {
            ui::begin_tooltip_nowrap();
            ui::draw_text(path.filename().string());
            ui::end_tooltip_nowrap();
        }
        ui::pop_id();
    }
}

class osc::SplashTab::Impl final : public TabPrivate {
public:

    explicit Impl(SplashTab& owner, Widget& parent_) :
        TabPrivate{owner, &parent_, OSC_ICON_HOME},
        m_MainMenuFileTab{parent_}
    {
        m_MainAppLogo.set_filter_mode(TextureFilterMode::Linear);
        m_CziLogo.set_filter_mode(TextureFilterMode::Linear);
        m_TudLogo.set_filter_mode(TextureFilterMode::Linear);
    }

    void on_mount()
    {
        // edge-case: reset the file tab whenever the splash screen is (re)mounted,
        // because actions within other tabs may have updated things like recently
        // used files etc. (#618)
        m_MainMenuFileTab = MainMenuFileTab{*parent()};

        App::upd().make_main_loop_waiting();
    }

    void on_unmount()
    {
        App::upd().make_main_loop_polling();
    }

    bool on_event(Event& e)
    {
        if (const auto* dropfile = dynamic_cast<const DropFileEvent*>(&e)) {
            if (dropfile->path().extension() == ".osim") {
                auto tab = std::make_unique<LoadingTab>(*parent(), dropfile->path());
                App::post_event<OpenTabEvent>(*parent(), std::move(tab));
                return true;
            }
        }
        return false;
    }

    void drawMainMenu()
    {
        m_MainMenuFileTab.onDraw();
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
        tabUIRect.p2.y -= max(m_TudLogo.device_independent_dimensions().y, m_CziLogo.device_independent_dimensions().y) - 2.0f*ui::get_style_panel_padding().y;

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

        ui::push_style_var(ui::StyleVar::WindowPadding, { 0.0f, 0.0f });
        ui::begin_panel("##splashscreenbackground", nullptr, ui::get_minimal_panel_flags());
        ui::pop_style_var();

        SceneRendererParams params{m_LastSceneRendererParams};
        params.virtual_pixel_dimensions = dimensions_of(viewportUIRect);
        params.device_pixel_ratio = App::settings().get_value<float>("graphics/render_scale", 1.0f) * App::get().main_window_device_pixel_ratio(),
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
        const Vec2 dims = dimensions_of(mmr);
        ui::set_next_panel_pos(mmr.p1);
        ui::set_next_panel_size({dims.x, -1.0f});
        ui::set_next_panel_size_constraints(dims, dims);

        if (ui::begin_panel("Splash screen", nullptr, ui::WindowFlag::NoTitleBar)) {
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
        if (ui::draw_menu_item(OSC_ICON_FILE " New Model")) {
            ActionNewModel(*parent());
        }
        if (ui::draw_menu_item(OSC_ICON_FOLDER_OPEN " Open Model")) {
            ActionOpenModel(*parent());
        }
        if (ui::draw_menu_item(OSC_ICON_FILE_IMPORT " Import Meshes")) {
            auto tab = std::make_unique<mi::MeshImporterTab>(*parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
        App::upd().add_frame_annotation("SplashTab/ImportMeshesMenuItem", ui::get_last_drawn_item_screen_rect());
        if (ui::draw_menu_item(OSC_ICON_BOOK " Open Documentation")) {
            open_url_in_os_default_web_browser(OpenSimCreatorApp::get().docs_url());
        }
    }

    void drawWorkflowsMenuSectionContent()
    {
        if (ui::draw_menu_item(OSC_ICON_FILE_IMPORT " Mesh Importer")) {
            auto tab = std::make_unique<mi::MeshImporterTab>(*parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }

        if (ui::draw_menu_item(OSC_ICON_MAGIC " Preview Experimental Data")) {
            auto tab = std::make_unique<PreviewExperimentalDataTab>(*parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
        if (ui::draw_menu_item(OSC_ICON_CUBE " Mesh Warping")) {
            auto tab = std::make_unique<MeshWarpingTab>(*parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
        App::upd().add_frame_annotation("SplashTab/MeshWarpingMenuItem", ui::get_last_drawn_item_screen_rect());

        if (ui::draw_menu_item(OSC_ICON_MAGIC " Model Warping (" OSC_ICON_MAGIC " experimental)")) {
            auto tab = std::make_unique<mow::ModelWarperTab>(*parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
        App::upd().add_frame_annotation("SplashTab/ModelWarpingMenuItem", ui::get_last_drawn_item_screen_rect());

        if (ui::draw_menu_item(OSC_ICON_ARROWS_ALT " Frame Definition (" OSC_ICON_TIMES " deprecated)")) {
            auto tab = std::make_unique<FrameDefinitionTab>(*parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
        ui::draw_tooltip_if_item_hovered("Frame Definition Workflow", "This feature is currently scheduled for deprecation. If you think it shouldn't be deprecated, then post a comment on GitHub issue #951.");
    }

    void drawRecentlyOpenedFilesMenuSectionContent(int& imguiID)
    {
        const auto recentFiles = App::singleton<RecentFiles>();
        if (not recentFiles->empty()) {
            for (const RecentFile& rf : *recentFiles) {
                DrawRecentOrExampleFileMenuItem(
                    rf.path,
                    *parent(),
                    imguiID
                );
            }
        }
        else {
            ui::push_style_color(ui::ColorVar::Text, Color::half_grey());
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
        if (not m_MainMenuFileTab.exampleOsimFiles.empty()) {

            ui::draw_text_disabled("Example Models");
            ui::draw_dummy({0.0f, 2.0f});

            for (const std::filesystem::path& examplePath : m_MainMenuFileTab.exampleOsimFiles) {
                DrawRecentOrExampleFileMenuItem(
                    examplePath,
                    *parent(),
                    imguiID
                );
            }
        }
    }

    void drawAttributationLogos()
    {
        const Rect viewportUIRect = ui::get_main_viewport_workspace_uiscreenspace_rect();
        Vec2 loc = viewportUIRect.p2;
        loc.x = loc.x - 2.0f*ui::get_style_panel_padding().x - m_CziLogo.device_independent_dimensions().x - 2.0f*ui::get_style_item_spacing().x - m_TudLogo.device_independent_dimensions().x;
        loc.y = loc.y - 2.0f*ui::get_style_panel_padding().y - max(m_CziLogo.device_independent_dimensions().y, m_TudLogo.device_independent_dimensions().y);

        ui::set_next_panel_pos(loc);
        ui::begin_panel("##czlogo", nullptr, ui::get_minimal_panel_flags());
        ui::draw_image(m_CziLogo);
        ui::end_panel();

        loc.x += m_CziLogo.device_independent_dimensions().x + 2.0f*ui::get_style_item_spacing().x;
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

        ui::DrawListView dl = ui::get_foreground_draw_list();
        const std::string text = App::get().application_name_with_version_and_buildid();
        dl.add_text(pos, Color::black(), text);
    }

    // for rendering the 3D scene
    PolarPerspectiveCamera m_Camera = GetSplashScreenDefaultPolarCamera();
    SceneRenderer scene_renderer_{
        *App::singleton<SceneCache>(App::resource_loader()),
    };
    SceneRendererParams m_LastSceneRendererParams = GetSplashScreenDefaultRenderParams(m_Camera);

    Texture2D m_MainAppLogo = load_texture2D_from_svg(App::load_resource("OpenSimCreator/textures/banner.svg"));
    Texture2D m_CziLogo = load_texture2D_from_svg(App::load_resource("OpenSimCreator/textures/chanzuckerberg_logo.svg"), 0.5f);
    Texture2D m_TudLogo = load_texture2D_from_svg(App::load_resource("OpenSimCreator/textures/tudelft_logo.svg"), 0.5f);

    // dimensions of stuff
    Vec2 m_SplashMenuMaxDims = {640.0f, 512.0f};
    Vec2 m_MainAppLogoDims =  m_MainAppLogo.device_independent_dimensions();
    Vec2 m_TopLogoPadding = {25.0f, 35.0f};

    // UI state
    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    LogViewer m_LogViewer;
};

osc::SplashTab::SplashTab(Widget& parent_) :
    Tab{std::make_unique<Impl>(*this, parent_)}
{}
void osc::SplashTab::impl_on_mount() { private_data().on_mount(); }
void osc::SplashTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::SplashTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::SplashTab::impl_on_draw_main_menu() { private_data().drawMainMenu(); }
void osc::SplashTab::impl_on_draw() { private_data().onDraw(); }
