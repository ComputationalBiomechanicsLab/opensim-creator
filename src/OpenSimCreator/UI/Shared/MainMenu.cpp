#include "MainMenu.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/Documents/Simulation/StoFileSimulation.h>
#include <OpenSimCreator/Platform/RecentFiles.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterTab.h>
#include <OpenSimCreator/UI/Simulation/SimulationTab.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppConfig.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/FilesystemHelpers.h>
#include <oscar/Utils/ParentPtr.h>

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <typeinfo>
#include <utility>

namespace rgs = std::ranges;

osc::MainMenuFileTab::MainMenuFileTab() :
    exampleOsimFiles
    {
        find_files_with_extensions_recursive(
            App::resource_filepath("models"),
            std::to_array({std::string_view{".osim"}})
        )
    }
{
    rgs::sort(exampleOsimFiles, is_filename_lexographically_greater_than);
}

void osc::MainMenuFileTab::onDraw(
    ParentPtr<IMainUIStateAPI> const& api,
    UndoableModelStatePair* maybeModel)
{
    // handle hotkeys enabled by just drawing the menu
    {
        const auto& io = ui::get_io();

        bool mod = ui::is_ctrl_or_super_down();

        if (mod && ui::is_key_pressed(ImGuiKey_N))
        {
            ActionNewModel(api);
        }
        else if (mod && ui::is_key_pressed(ImGuiKey_O))
        {
            ActionOpenModel(api);
        }
        else if (maybeModel && mod && io.KeyShift && ui::is_key_pressed(ImGuiKey_S))
        {
            ActionSaveCurrentModelAs(*maybeModel);
        }
        else if (maybeModel && mod && ui::is_key_pressed(ImGuiKey_S))
        {
            ActionSaveModel(*api, *maybeModel);
        }
        else if (maybeModel && ui::is_key_pressed(ImGuiKey_F5))
        {
            ActionReloadOsimFromDisk(*maybeModel, *App::singleton<SceneCache>());
        }
    }

    // draw "save as", if necessary
    if (maybeSaveChangesPopup)
    {
        maybeSaveChangesPopup->on_draw();
    }

    if (!ui::begin_menu("File"))
    {
        return;
    }

    if (ui::draw_menu_item(ICON_FA_FILE " New", "Ctrl+N"))
    {
        ActionNewModel(api);
    }

    if (ui::draw_menu_item(ICON_FA_FOLDER_OPEN " Open", "Ctrl+O"))
    {
        ActionOpenModel(api);
    }

    int imgui_id = 0;

    auto recentFiles = App::singleton<RecentFiles>();
    if (ui::begin_menu(ICON_FA_FOLDER_OPEN " Open Recent", !recentFiles->empty()))
    {
        // iterate in reverse: recent files are stored oldest --> newest
        for (const RecentFile& rf : *recentFiles)
        {
            ui::push_id(++imgui_id);
            if (ui::draw_menu_item(rf.path.filename().string()))
            {
                ActionOpenModel(api, rf.path);
            }
            ui::pop_id();
        }

        ui::end_menu();
    }

    if (ui::begin_menu(ICON_FA_FOLDER_OPEN " Open Example"))
    {
        for (const std::filesystem::path& ex : exampleOsimFiles)
        {
            ui::push_id(++imgui_id);
            if (ui::draw_menu_item(ex.filename().string()))
            {
                ActionOpenModel(api, ex);
            }
            ui::pop_id();
        }

        ui::end_menu();
    }

    ui::draw_separator();

    if (ui::draw_menu_item(ICON_FA_FOLDER_OPEN " Load Motion", {}, false, maybeModel != nullptr))
    {
        std::optional<std::filesystem::path> maybePath = prompt_user_to_select_file({"sto", "mot"});
        if (maybePath && maybeModel)
        {
            try
            {
                std::unique_ptr<OpenSim::Model> cpy = std::make_unique<OpenSim::Model>(maybeModel->getModel());
                InitializeModel(*cpy);
                InitializeState(*cpy);

                api->add_and_select_tab<SimulationTab>(api, std::make_shared<Simulation>(StoFileSimulation{std::move(cpy), *maybePath, maybeModel->getFixupScaleFactor()}));
            }
            catch (const std::exception& ex)
            {
                log_error("encountered error while trying to load an STO file against the model: %s", ex.what());
            }
        }
    }

    ui::draw_separator();

    if (ui::draw_menu_item(ICON_FA_SAVE " Save", "Ctrl+S", false, maybeModel != nullptr))
    {
        if (maybeModel)
        {
            ActionSaveModel(*api, *maybeModel);
        }
    }

    if (ui::draw_menu_item(ICON_FA_SAVE " Save As", "Shift+Ctrl+S", false, maybeModel != nullptr))
    {
        if (maybeModel)
        {
            ActionSaveCurrentModelAs(*maybeModel);
        }
    }

    ui::draw_separator();

    {
        const bool modelHasBackingFile = maybeModel != nullptr && HasInputFileName(maybeModel->getModel());

        if (ui::draw_menu_item(ICON_FA_RECYCLE " Reload", "F5", false, modelHasBackingFile) && maybeModel)
        {
            ActionReloadOsimFromDisk(*maybeModel, *App::singleton<SceneCache>());
        }
        ui::draw_tooltip_if_item_hovered("Reload", "Attempts to reload the osim file from scratch. This can be useful if (e.g.) editing third-party files that OpenSim Creator doesn't automatically track.");

        if (ui::draw_menu_item(ICON_FA_CLIPBOARD " Copy .osim path to clipboard", {}, false, modelHasBackingFile) && maybeModel)
        {
            ActionCopyModelPathToClipboard(*maybeModel);
        }
        ui::draw_tooltip_if_item_hovered("Copy .osim path to clipboard", "Copies the absolute path to the model's .osim file into your clipboard.\n\nThis is handy if you want to (e.g.) load the osim via a script, open it from the command line in another app, etc.");

        if (ui::draw_menu_item(ICON_FA_FOLDER " Open .osim's parent directory", {}, false, modelHasBackingFile) && maybeModel)
        {
            ActionOpenOsimParentDirectory(*maybeModel);
        }

        if (ui::draw_menu_item(ICON_FA_LINK " Open .osim in external editor", {}, false, modelHasBackingFile) && maybeModel)
        {
            ActionOpenOsimInExternalEditor(*maybeModel);
        }
        ui::draw_tooltip_if_item_hovered("Open .osim in external editor", "Open the .osim file currently being edited in an external text editor. The editor that's used depends on your operating system's default for opening .osim files.");
    }

    // reload
    // copy path to clipboard
    // parent dir
    // external editor

    ui::draw_separator();

    if (ui::draw_menu_item(ICON_FA_MAGIC " Import Meshes"))
    {
        api->add_and_select_tab<mi::MeshImporterTab>(api);
    }
    App::upd().add_frame_annotation("MainMenu/ImportMeshesMenuItem", ui::get_last_drawn_item_screen_rect());



    if (ui::draw_menu_item(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q"))
    {
        App::upd().request_quit();
    }

    ui::end_menu();
}


void osc::MainMenuAboutTab::onDraw()
{
    if (!ui::begin_menu("About"))
    {
        return;
    }

    constexpr float menuWidth = 400;
    ui::draw_dummy({menuWidth, 0});

    ui::draw_text_unformatted("graphics");
    ui::same_line();
    ui::draw_help_marker("OSMV's global graphical settings");
    ui::draw_separator();
    ui::draw_dummy({0.0f, 0.5f});
    {
        ui::set_num_columns(2);

        ui::draw_text_unformatted("FPS");
        ui::next_column();
        ui::draw_text("%.0f", static_cast<double>(ui::get_io().Framerate));
        ui::next_column();

        ui::draw_text_unformatted("MSXAA");
        ui::same_line();
        ui::draw_help_marker("the log_level_ of MultiSample Anti-Aliasing to use. This only affects 3D renders *within* the UI, not the whole UI (panels etc. will not be affected)");
        ui::next_column();
        {
            const AntiAliasingLevel current = App::get().anti_aliasing_level();
            const AntiAliasingLevel max = App::get().max_anti_aliasing_level();

            if (ui::begin_combobox("##msxaa", to_string(current)))
            {
                for (AntiAliasingLevel l = AntiAliasingLevel::min(); l <= max; ++l)
                {
                    bool selected = l == current;
                    if (ui::draw_selectable(to_string(l), &selected))
                    {
                        App::upd().set_anti_aliasing_level(l);
                    }
                }
                ui::end_combobox();
            }
        }
        ui::next_column();

        ui::draw_text_unformatted("window");
        ui::next_column();

        if (ui::draw_button(ICON_FA_EXPAND " fullscreen"))
        {
            App::upd().make_fullscreen();
        }
        if (ui::draw_button(ICON_FA_EXPAND " windowed fullscreen"))
        {
            App::upd().make_windowed_fullscreen();
        }
        if (ui::draw_button(ICON_FA_WINDOW_RESTORE " windowed"))
        {
            App::upd().make_windowed();
        }
        ui::next_column();

        ui::draw_text_unformatted("VSYNC");
        ui::same_line();
        ui::draw_help_marker("whether the backend uses vertical sync (VSYNC), which will cap the rendering FPS to your monitor's refresh rate");
        ui::next_column();

        bool enabled = App::get().is_vsync_enabled();
        if (ui::draw_checkbox("##vsynccheckbox", &enabled)) {
            App::upd().set_vsync_enabled(enabled);
        }
        ui::next_column();

        ui::set_num_columns();
    }

    ui::draw_dummy({0.0f, 2.0f});
    ui::draw_text_unformatted("properties");
    ui::same_line();
    ui::draw_help_marker("general software properties: useful information for bug reporting etc.");
    ui::draw_separator();
    ui::draw_dummy({0.0f, 0.5f});
    {
        const AppMetadata& metadata = App::get().metadata();

        ui::set_num_columns(2);

        ui::draw_text_unformatted("VERSION");
        ui::next_column();
        ui::draw_text_unformatted(metadata.maybe_version_string().value_or("(not known)"));
        ui::next_column();

        ui::draw_text_unformatted("BUILD_ID");
        ui::next_column();
        ui::draw_text_unformatted(metadata.maybe_build_id().value_or("(not known)"));
        ui::next_column();

        ui::draw_text_unformatted("GRAPHICS_VENDOR");
        ui::next_column();
        ui::draw_text(App::get().graphics_backend_vendor_string());
        ui::next_column();

        ui::draw_text_unformatted("GRAPHICS_RENDERER");
        ui::next_column();
        ui::draw_text(App::get().graphics_backend_renderer_string());
        ui::next_column();

        ui::draw_text_unformatted("GRAPHICS_RENDERER_VERSION");
        ui::next_column();
        ui::draw_text(App::get().graphics_backend_version_string());
        ui::next_column();

        ui::draw_text_unformatted("GRAPHICS_SHADER_VERSION");
        ui::next_column();
        ui::draw_text(App::get().graphics_backend_shading_language_version_string());
        ui::next_column();

        ui::set_num_columns(1);
    }

    ui::draw_dummy({0.0f, 2.5f});
    ui::draw_text_unformatted("debugging utilities:");
    ui::same_line();
    ui::draw_help_marker("standard utilities that can help with development, debugging, etc.");
    ui::draw_separator();
    ui::draw_dummy({0.0f, 0.5f});
    int id = 0;
    {
        ui::set_num_columns(2);

        ui::draw_text_unformatted("OSC Install Location");
        ui::same_line();
        ui::draw_help_marker("opens OSC's installation location in your OS's default file browser");
        ui::next_column();
        ui::push_id(id++);
        if (ui::draw_button(ICON_FA_FOLDER " open"))
        {
            open_file_in_os_default_application(App::get().executable_dir());
        }
        ui::pop_id();
        ui::next_column();

        ui::draw_text_unformatted("User Data Dir");
        ui::same_line();
        ui::draw_help_marker("opens your OSC user data directory in your OS's default file browser");
        ui::next_column();
        ui::push_id(id++);
        if (ui::draw_button(ICON_FA_FOLDER " open")) {
            open_file_in_os_default_application(App::get().user_data_dir());
        }
        ui::pop_id();
        ui::next_column();

        ui::draw_text_unformatted("Debug mode");
        ui::same_line();
        ui::draw_help_marker("Toggles whether the application is in debug mode or not: enabling this can reveal more inforamtion about bugs");
        ui::next_column();
        {
            bool appIsInDebugMode = App::get().is_in_debug_mode();
            if (ui::draw_checkbox("##debugmodecheckbox", &appIsInDebugMode)) {
                App::upd().set_debug_mode(appIsInDebugMode);
            }
        }

        ui::set_num_columns();
    }

    ui::draw_dummy({0.0f, 2.5f});
    ui::draw_text_unformatted("useful links:");
    ui::same_line();
    ui::draw_help_marker("links to external sites that might be useful");
    ui::draw_separator();
    ui::draw_dummy({0.0f, 0.5f});
    {
        ui::set_num_columns(2);

        ui::draw_text_unformatted("OpenSim Creator Documentation");
        ui::next_column();
        ui::push_id(id++);
        if (ui::draw_button(ICON_FA_LINK " open"))
        {
            open_url_in_os_default_web_browser(App::config().docs_url());
        }
        ui::draw_tooltip_body_only_if_item_hovered("this will open the (locally installed) documentation in a separate browser window");
        ui::pop_id();
        ui::next_column();

        if (auto repoURL = App::get().metadata().maybe_repository_url())
        {
            ui::draw_text_unformatted("OpenSim Creator Repository");
            ui::next_column();
            ui::push_id(id++);
            if (ui::draw_button(ICON_FA_LINK " open"))
            {
                open_file_in_os_default_application(std::filesystem::path{std::string_view{*repoURL}});
            }
            ui::draw_tooltip_body_only_if_item_hovered("this will open the repository homepage in a separate browser window");
            ui::pop_id();
            ui::next_column();
        }

        if (auto helpURL = App::get().metadata().maybe_help_url())
        {
            ui::draw_text_unformatted("OpenSim Creator Help");
            ui::next_column();
            ui::push_id(id++);
            if (ui::draw_button(ICON_FA_LINK " open"))
            {
                open_file_in_os_default_application(std::filesystem::path{std::string_view{*helpURL}});
            }
            ui::draw_tooltip_body_only_if_item_hovered("this will open the help/discussion page in a separate browser window");
            ui::pop_id();
            ui::next_column();
        }

        ui::draw_text_unformatted("OpenSim Documentation");
        ui::next_column();
        ui::push_id(id++);
        if (ui::draw_button(ICON_FA_LINK " open"))
        {
            open_file_in_os_default_application("https://simtk-confluence.stanford.edu/display/OpenSim/Documentation");
        }
        ui::draw_tooltip_body_only_if_item_hovered("this will open the documentation in a separate browser window");
        ui::pop_id();
        ui::next_column();

        ui::set_num_columns();
    }

    ui::end_menu();
}
