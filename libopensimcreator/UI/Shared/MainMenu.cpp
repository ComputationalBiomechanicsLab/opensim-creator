#include "MainMenu.h"

#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/Documents/Simulation/Simulation.h>
#include <libopensimcreator/Documents/Simulation/StoFileSimulation.h>
#include <libopensimcreator/Platform/OpenSimCreatorApp.h>
#include <libopensimcreator/Platform/RecentFiles.h>
#include <libopensimcreator/UI/MeshImporter/MeshImporterTab.h>
#include <libopensimcreator/UI/PreviewExperimentalData/PreviewExperimentalDataTab.h>
#include <libopensimcreator/UI/Simulation/SimulationTab.h>
#include <libopensimcreator/UI/SplashTab.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/AppMetadata.h>
#include <liboscar/Platform/FileDialogFilter.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/Platform/os.h>
#include <liboscar/UI/Events/CloseTabEvent.h>
#include <liboscar/UI/Events/OpenTabEvent.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/FilesystemHelpers.h>
#include <liboscar/Utils/StringHelpers.h>
#include <OpenSim/Simulation/Model/Model.h>

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
using namespace osc;

namespace
{

    const std::span<const FileDialogFilter> GetMotionFileFilters()
    {
        static const auto s_MotionFileFilters = std::to_array<FileDialogFilter>({
            FileDialogFilter{"OpenSim Storage File (.sto)", "sto"},
            FileDialogFilter{"OpenSim/SIMM Motion File (.mot)", "mot"},
            FileDialogFilter::all_files(),
        });

        return s_MotionFileFilters;
    }

    void LoadMotionAgainstModel(std::shared_ptr<IModelStatePair> model, Widget* parent)
    {
        if (not model) {
            return;  // Nothing to load the motion against.
        }

        if (not parent) {
            return;  // Nothing to post the "open motion tab" event to.
        }

        // Asynchronously ask the user to select a motion file and then load the motion
        // file against the model and show the result in a new tab.
        App::upd().prompt_user_to_select_file_async(
            [model, parent_ref = parent->weak_ref()](FileDialogResponse response)
            {
                if (response.size() != 1) {
                    return;  // Error or user somehow selected too many files.
                }

                if (parent_ref.expired()) {
                    log_error("Parent widget was destroyed while the user was selecting a file");
                    return;
                }

                try {
                    std::unique_ptr<OpenSim::Model> cpy = std::make_unique<OpenSim::Model>(model->getModel());
                    InitializeModel(*cpy);
                    InitializeState(*cpy);

                    auto simulation = std::make_shared<Simulation>(
                        StoFileSimulation{std::move(cpy),
                        response.front(),
                        model->getFixupScaleFactor(),
                        model->tryUpdEnvironment()
                    });
                    auto tab = std::make_unique<SimulationTab>(parent_ref.get(), simulation);
                    App::post_event<OpenTabEvent>(*parent_ref, std::move(tab));
                }
                catch (const std::exception& ex) {
                    log_error("encountered error while trying to load an STO file against the model: %s", ex.what());
                }
            },
            GetMotionFileFilters()
        );
    }
}

osc::MainMenuFileTab::MainMenuFileTab(Widget* parent) :
    Widget{parent},
    exampleOsimFiles
    {
        find_files_with_extensions_recursive(
            App::resource_filepath("OpenSimCreator/models"),
            std::to_array({std::string_view{".osim"}})
        )
    }
{
    rgs::sort(exampleOsimFiles, is_filename_lexicographically_greater_than);
}

void osc::MainMenuFileTab::onDraw(std::shared_ptr<IModelStatePair> maybeModel)
{
    auto undoableModel = std::dynamic_pointer_cast<UndoableModelStatePair>(maybeModel);

    // handle hotkeys enabled by just drawing the menu
    {
        bool mod = ui::is_ctrl_or_super_down();

        if (mod and ui::is_key_pressed(Key::N)) {
            if (parent()) {
                ActionNewModel(*parent());
            }
        }
        else if (mod and ui::is_key_pressed(Key::O)) {
            if (parent()) {
                ActionOpenModel(*parent());
            }
        }
        else if (undoableModel and mod and ui::is_shift_down() and ui::is_key_pressed(Key::S)) {
            ActionSaveCurrentModelAs(*undoableModel);
        }
        else if (undoableModel and mod and ui::is_key_pressed(Key::S)) {
            ActionSaveModel(*undoableModel);
        }
        else if (undoableModel and ui::is_key_pressed(Key::F5)) {
            ActionReloadOsimFromDisk(*undoableModel, *App::singleton<SceneCache>());
        }
    }

    if (not ui::begin_menu("File")) {
        return;
    }

    if (ui::draw_menu_item(OSC_ICON_FILE " New", KeyModifier::Ctrl | Key::N)) {
        if (parent()) {
            ActionNewModel(*parent());
        }
    }

    if (ui::draw_menu_item(OSC_ICON_FOLDER_OPEN " Open", KeyModifier::Ctrl | Key::O)) {
        if (parent()) {
            ActionOpenModel(*parent());
        }
    }

    int imgui_id = 0;

    auto recentFiles = App::singleton<RecentFiles>();
    if (ui::begin_menu(OSC_ICON_FOLDER_OPEN " Open Recent", !recentFiles->empty())) {
        // iterate in reverse: recent files are stored oldest --> newest
        for (const RecentFile& rf : *recentFiles) {
            ui::push_id(++imgui_id);
            if (ui::draw_menu_item(rf.path.filename().string())) {
                if (parent()) {
                    ActionOpenModel(*parent(), rf.path);
                }
            }
            ui::pop_id();
        }

        ui::end_menu();
    }

    if (ui::begin_menu(OSC_ICON_FOLDER_OPEN " Open Example")) {
        for (const std::filesystem::path& ex : exampleOsimFiles) {
            ui::push_id(++imgui_id);
            if (ui::draw_menu_item(ex.filename().string())) {
                if (parent()) {
                    ActionOpenModel(*parent(), ex);
                }
            }
            ui::pop_id();
        }

        ui::end_menu();
    }

    ui::draw_separator();

    if (ui::draw_menu_item(OSC_ICON_FOLDER_OPEN " Load Motion", {}, false, maybeModel != nullptr)) {
        LoadMotionAgainstModel(maybeModel, parent());
    }

    ui::draw_separator();

    if (ui::draw_menu_item(OSC_ICON_SAVE " Save", KeyModifier::Ctrl | Key::S, false, undoableModel != nullptr)) {
        if (undoableModel) {
            ActionSaveModel(*undoableModel);
        }
    }

    if (ui::draw_menu_item(OSC_ICON_SAVE " Save As", KeyModifier::Ctrl | KeyModifier::Shift | Key::S, false, undoableModel != nullptr)) {
        if (undoableModel) {
            ActionSaveCurrentModelAs(*undoableModel);
        }
    }

    {
        Tab* parentTab = first_ancestor_of_type<Tab>();
        // HACK: `SplashTab` is the only not-closeable tab
        const bool enabled = (undoableModel != nullptr) and (dynamic_cast<const SplashTab*>(parentTab) == nullptr);
        if (ui::draw_menu_item(OSC_ICON_TIMES " Close", KeyModifier::Ctrl | Key::W, false, enabled)) {
            App::post_event<CloseTabEvent>(*parentTab, parentTab->id());
        }
    }

    ui::draw_separator();

    {
        const bool modelHasBackingFile = maybeModel != nullptr && HasInputFileName(maybeModel->getModel());

        if (ui::draw_menu_item(OSC_ICON_RECYCLE " Reload", Key::F5, false, undoableModel != nullptr and undoableModel->canUpdModel() and modelHasBackingFile) and undoableModel != nullptr) {
            ActionReloadOsimFromDisk(*undoableModel, *App::singleton<SceneCache>());
        }
        ui::draw_tooltip_if_item_hovered("Reload", "Attempts to reload the osim file from scratch. This can be useful if (e.g.) editing third-party files that OpenSim Creator doesn't automatically track.");

        if (ui::draw_menu_item(OSC_ICON_CLIPBOARD " Copy .osim path to clipboard", {}, false, undoableModel != nullptr and modelHasBackingFile) and undoableModel != nullptr) {
            ActionCopyModelPathToClipboard(*undoableModel);
        }
        ui::draw_tooltip_if_item_hovered("Copy .osim path to clipboard", "Copies the absolute path to the model's .osim file into your clipboard.\n\nThis is handy if you want to (e.g.) load the osim via a script, open it from the command line in another app, etc.");

        if (ui::draw_menu_item(OSC_ICON_FOLDER " Open .osim's parent directory", {}, false, modelHasBackingFile) && maybeModel) {
            ActionOpenOsimParentDirectory(*maybeModel);
        }

        if (ui::draw_menu_item(OSC_ICON_LINK " Open .osim in external editor", {}, false, modelHasBackingFile) && maybeModel) {
            ActionOpenOsimInExternalEditor(*maybeModel);
        }
        ui::draw_tooltip_if_item_hovered("Open .osim in external editor", "Open the .osim file currently being edited in an external text editor. The editor that's used depends on your operating system's default for opening .osim files.");
    }

    // reload
    // copy path to clipboard
    // parent dir
    // external editor

    ui::draw_separator();

    if (ui::draw_menu_item(OSC_ICON_FILE_IMPORT " Import Meshes")) {
        if (parent()) {
            auto tab = std::make_unique<mi::MeshImporterTab>(parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
    }
    App::upd().add_frame_annotation("MainMenu/ImportMeshesMenuItem", ui::get_last_drawn_item_screen_rect());
    if (ui::draw_menu_item(OSC_ICON_BEZIER_CURVE " Preview Experimental Data")) {
        if (parent()) {
            auto tab = std::make_unique<PreviewExperimentalDataTab>(parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
    }

    ui::draw_separator();

    if (ui::draw_menu_item(OSC_ICON_TIMES_CIRCLE " Quit", KeyModifier::Ctrl | Key::Q)) {
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

    ui::draw_text("graphics");
    ui::same_line();
    ui::draw_help_marker("OSMV's global graphical settings");
    ui::draw_separator();
    ui::draw_dummy({0.0f, 0.5f});
    {
        ui::set_num_columns(2);

        ui::draw_text("FPS");
        ui::next_column();
        ui::draw_text("%.0f", static_cast<double>(ui::get_framerate()));
        ui::next_column();

        ui::draw_text("MSXAA");
        ui::same_line();
        ui::draw_help_marker("the log_level_ of MultiSample Anti-Aliasing to use. This only affects 3D renders *within* the UI, not the whole UI (panels etc. will not be affected)");
        ui::next_column();
        {
            const AntiAliasingLevel current = App::get().anti_aliasing_level();
            const AntiAliasingLevel max = App::get().max_anti_aliasing_level();

            if (ui::begin_combobox("##msxaa", stream_to_string(current)))
            {
                for (AntiAliasingLevel l = AntiAliasingLevel::min(); l <= max; ++l)
                {
                    bool selected = l == current;
                    if (ui::draw_selectable(stream_to_string(l), &selected))
                    {
                        App::upd().set_anti_aliasing_level(l);
                    }
                }
                ui::end_combobox();
            }
        }
        ui::next_column();

        ui::draw_text("window");
        ui::next_column();

        if (ui::draw_button(OSC_ICON_EXPAND " fullscreen")) {
            App::upd().make_windowed_fullscreen();
        }
        if (ui::draw_button(OSC_ICON_WINDOW_RESTORE " windowed")) {
            App::upd().make_windowed();
        }
        ui::next_column();

        ui::draw_text("VSYNC");
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
    ui::draw_text("properties");
    ui::same_line();
    ui::draw_help_marker("general software properties: useful information for bug reporting etc.");
    ui::draw_separator();
    ui::draw_dummy({0.0f, 0.5f});
    {
        const AppMetadata& metadata = App::get().metadata();

        ui::set_num_columns(2);

        ui::draw_text("VERSION");
        ui::next_column();
        ui::draw_text(metadata.version_string().value_or("(not known)"));
        ui::next_column();

        ui::draw_text("BUILD_ID");
        ui::next_column();
        ui::draw_text(metadata.build_id().value_or("(not known)"));
        ui::next_column();

        ui::draw_text("GRAPHICS_VENDOR");
        ui::next_column();
        ui::draw_text(App::get().graphics_backend_vendor_string());
        ui::next_column();

        ui::draw_text("GRAPHICS_RENDERER");
        ui::next_column();
        ui::draw_text(App::get().graphics_backend_renderer_string());
        ui::next_column();

        ui::draw_text("GRAPHICS_RENDERER_VERSION");
        ui::next_column();
        ui::draw_text(App::get().graphics_backend_version_string());
        ui::next_column();

        ui::draw_text("GRAPHICS_SHADER_VERSION");
        ui::next_column();
        ui::draw_text(App::get().graphics_backend_shading_language_version_string());
        ui::next_column();

        ui::set_num_columns(1);
    }

    ui::draw_dummy({0.0f, 2.5f});
    ui::draw_text("debugging utilities:");
    ui::same_line();
    ui::draw_help_marker("standard utilities that can help with development, debugging, etc.");
    ui::draw_separator();
    ui::draw_dummy({0.0f, 0.5f});
    int id = 0;
    {
        ui::set_num_columns(2);

        ui::draw_text("OSC Install Location");
        ui::same_line();
        ui::draw_help_marker("opens OSC's installation location in your OS's default file browser");
        ui::next_column();
        ui::push_id(id++);
        if (ui::draw_button(OSC_ICON_FOLDER " open"))
        {
            open_file_in_os_default_application(App::get().executable_directory());
        }
        ui::pop_id();
        ui::next_column();

        ui::draw_text("User Data Dir");
        ui::same_line();
        ui::draw_help_marker("opens your OSC user data directory in your OS's default file browser");
        ui::next_column();
        ui::push_id(id++);
        if (ui::draw_button(OSC_ICON_FOLDER " open")) {
            open_file_in_os_default_application(App::get().user_data_directory());
        }
        ui::pop_id();
        ui::next_column();

        ui::draw_text("Debug mode");
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
    ui::draw_text("useful links:");
    ui::same_line();
    ui::draw_help_marker("links to external sites that might be useful");
    ui::draw_separator();
    ui::draw_dummy({0.0f, 0.5f});
    {
        ui::set_num_columns(2);

        ui::draw_text("OpenSim Creator Documentation");
        ui::next_column();
        ui::push_id(id++);
        if (ui::draw_button(OSC_ICON_LINK " open"))
        {
            open_url_in_os_default_web_browser(OpenSimCreatorApp::get().docs_url());
        }
        ui::draw_tooltip_body_only_if_item_hovered("this will open the (locally installed) documentation in a separate browser window");
        ui::pop_id();
        ui::next_column();

        if (auto repoURL = App::get().metadata().repository_url())
        {
            ui::draw_text("OpenSim Creator Repository");
            ui::next_column();
            ui::push_id(id++);
            if (ui::draw_button(OSC_ICON_LINK " open"))
            {
                open_file_in_os_default_application(std::filesystem::path{std::string_view{*repoURL}});
            }
            ui::draw_tooltip_body_only_if_item_hovered("this will open the repository homepage in a separate browser window");
            ui::pop_id();
            ui::next_column();
        }

        if (auto helpURL = App::get().metadata().help_url())
        {
            ui::draw_text("OpenSim Creator Help");
            ui::next_column();
            ui::push_id(id++);
            if (ui::draw_button(OSC_ICON_LINK " open"))
            {
                open_file_in_os_default_application(std::filesystem::path{std::string_view{*helpURL}});
            }
            ui::draw_tooltip_body_only_if_item_hovered("this will open the help/discussion page in a separate browser window");
            ui::pop_id();
            ui::next_column();
        }

        ui::draw_text("OpenSim Documentation");
        ui::next_column();
        ui::push_id(id++);
        if (ui::draw_button(OSC_ICON_LINK " open"))
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
