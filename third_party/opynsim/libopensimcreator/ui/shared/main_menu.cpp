#include "main_menu.h"

#include <libopensimcreator/documents/model/environment.h>
#include <libopensimcreator/documents/model/model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/documents/simulation/simulation.h>
#include <libopensimcreator/documents/simulation/sto_file_simulation.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/platform/recent_files.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_tab.h>
#include <libopensimcreator/ui/preview_experimental_data/preview_experimental_data_tab.h>
#include <libopensimcreator/ui/simulation/simulation_tab.h>
#include <libopensimcreator/ui/splash_tab.h>

#include <libopynsim/documents/file_filters.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_metadata.h>
#include <liboscar/platform/log.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/events/close_tab_event.h>
#include <liboscar/ui/events/open_tab_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/filesystem_helpers.h>
#include <liboscar/utilities/string_helpers.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <typeinfo>
#include <utility>

namespace rgs = std::ranges;
using namespace osc;

namespace
{
    void load_motion_against_model(
        const std::shared_ptr<opyn::ModelStatePair>& model, Widget* parent)
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
            [model, parent_ref = parent->weak_ref()](const FileDialogResponse& response) {
                if (response.size() != 1) {
                    return;  // Error or user somehow selected too many files.
                }

                if (parent_ref.expired()) {
                    log_error("Parent widget was destroyed while the user was selecting a file");
                    return;
                }

                try {
                    std::unique_ptr<OpenSim::Model> cpy =
                        std::make_unique<OpenSim::Model>(model->get_model());
                    opyn::initialize_model(*cpy);
                    opyn::initialize_state(*cpy);

                    std::shared_ptr<Environment> env;
                    if (auto model_with_env =
                            std::dynamic_pointer_cast<ModelStatePairWithSharedEnvironment>(model)) {
                        env = model_with_env->try_upd_environment();
                    } else {
                        env = std::make_shared<Environment>();
                    }

                    auto simulation = std::make_shared<Simulation>(StoFileSimulation{
                        std::move(cpy),
                        response.front(),
                        model->get_fixup_scale_factor(),
                        std::move(env)});
                    auto tab = std::make_unique<SimulationTab>(parent_ref.get(), simulation);
                    App::post_event<OpenTabEvent>(*parent_ref, std::move(tab));
                } catch (const std::exception& ex) {
                    log_error(
                        "encountered error while trying to load an STO file against the model: {}",
                        ex.what());
                }
            },
            opyn::get_motion_file_filters());
    }

    std::vector<std::filesystem::path> try_load_example_osim_files()
    {
        std::vector<std::filesystem::path> rv;

        const ResourcePath example_osims_resource_path{"OpenSimCreator/models"};
        if (const auto example_osims_directory_path =
                App::resource_filepath(example_osims_resource_path)) {

            rv = find_files_with_extensions_recursive(
                *example_osims_directory_path, std::to_array({std::string_view{".osim"}}));
        } else {
            log_warn(
                "{}: no such directory: cannot iterate through example models: falling back to an empty list",
                example_osims_resource_path.string());
        }

        rgs::sort(rv, is_filename_lexicographically_greater_than);

        return rv;
    }
}

osc::MainMenuFileTab::MainMenuFileTab(Widget* parent) :
    Widget{parent},
    example_osim_files{try_load_example_osim_files()}
{}

void osc::MainMenuFileTab::on_draw(std::shared_ptr<opyn::ModelStatePair> maybe_model)
{
    auto undoable_model = std::dynamic_pointer_cast<UndoableModelStatePair>(maybe_model);

    // handle hotkeys enabled by just drawing the menu
    {
        bool mod = ui::is_ctrl_or_super_down();

        if (mod and ui::is_key_pressed(Key::N)) {
            if (parent()) {
                action_new_model(*parent());
            }
        } else if (mod and ui::is_key_pressed(Key::O)) {
            if (parent()) {
                action_open_model(*parent());
            }
        } else if (undoable_model and mod and ui::is_shift_down() and ui::is_key_pressed(Key::S)) {
            action_save_current_model_as(undoable_model);
        } else if (undoable_model and mod and ui::is_key_pressed(Key::S)) {
            action_save_model_async(undoable_model);
        } else if (undoable_model and ui::is_key_pressed(Key::F5)) {
            action_reload_osim_from_disk(*undoable_model, *App::singleton<SceneCache>());
        }
    }

    if (not ui::begin_menu("File")) {
        return;
    }

    if (ui::draw_menu_item(MSMICONS_FILE " New", ModifierKey::Ctrl | Key::N)) {
        if (parent()) {
            action_new_model(*parent());
        }
    }

    if (ui::draw_menu_item(MSMICONS_FOLDER_OPEN " Open", ModifierKey::Ctrl | Key::O)) {
        if (parent()) {
            action_open_model(*parent());
        }
    }

    int imgui_id = 0;

    auto recent_files = App::singleton<RecentFiles>();
    if (ui::begin_menu(MSMICONS_FOLDER_OPEN " Open Recent", !recent_files->empty())) {
        // iterate in reverse: recent files are stored oldest --> newest
        for (const RecentFile& rf : *recent_files) {
            ui::push_id(++imgui_id);
            if (ui::draw_menu_item(rf.path.filename().string())) {
                if (parent()) {
                    action_open_model(*parent(), rf.path);
                }
            }
            ui::pop_id();
        }

        ui::end_menu();
    }

    if (ui::begin_menu(MSMICONS_FOLDER_OPEN " Open Example")) {
        for (const std::filesystem::path& ex : example_osim_files) {
            ui::push_id(++imgui_id);
            if (ui::draw_menu_item(ex.filename().string())) {
                if (parent()) {
                    action_open_model(*parent(), ex);
                }
            }
            ui::pop_id();
        }

        ui::end_menu();
    }

    ui::draw_separator();

    if (ui::draw_menu_item(
            MSMICONS_FOLDER_OPEN " Load Motion", std::nullopt, false, maybe_model != nullptr)) {
        load_motion_against_model(maybe_model, parent());
    }

    ui::draw_separator();

    if (ui::draw_menu_item(
            MSMICONS_SAVE " Save", ModifierKey::Ctrl | Key::S, false, undoable_model != nullptr)) {
        if (undoable_model) {
            action_save_model_async(undoable_model);
        }
    }

    if (ui::draw_menu_item(
            MSMICONS_SAVE " Save As",
            ModifierKey::Ctrl | ModifierKey::Shift | Key::S,
            false,
            undoable_model != nullptr)) {
        if (undoable_model) {
            action_save_current_model_as(undoable_model);
        }
    }

    {
        Tab* parent_tab = first_ancestor_of_type<Tab>();
        // HACK: `SplashTab` is the only not-closeable tab
        const bool enabled =
            (undoable_model != nullptr) and (dynamic_cast<const SplashTab*>(parent_tab) == nullptr);
        if (ui::draw_menu_item(
                MSMICONS_TIMES " Close", ModifierKey::Ctrl | Key::W, false, enabled)) {
            App::post_event<CloseTabEvent>(*parent_tab, parent_tab->id());
        }
    }

    ui::draw_separator();

    {
        const bool model_has_backing_file =
            maybe_model != nullptr && opyn::has_input_file_name(maybe_model->get_model());

        if (ui::draw_menu_item(
                MSMICONS_RECYCLE " Reload",
                Key::F5,
                false,
                undoable_model != nullptr and undoable_model->can_upd_model() and
                    model_has_backing_file) and
            undoable_model != nullptr) {

            action_reload_osim_from_disk(*undoable_model, *App::singleton<SceneCache>());
        }
        ui::draw_tooltip_if_item_hovered(
            "Reload",
            "Attempts to reload the osim file from scratch. This can be useful if (e.g.) editing third-party files that OpenSim Creator doesn't automatically track.");

        if (ui::draw_menu_item(
                MSMICONS_CLIPBOARD " Copy .osim path to clipboard",
                std::nullopt,
                false,
                undoable_model != nullptr and model_has_backing_file) and
            undoable_model != nullptr) {

            action_copy_model_path_to_clipboard(*undoable_model);
        }
        ui::draw_tooltip_if_item_hovered(
            "Copy .osim path to clipboard",
            "Copies the absolute path to the model's .osim file into your clipboard.\n\nThis is handy if you want to (e.g.) load the osim via a script, open it from the command line in another app, etc.");

        if (ui::draw_menu_item(
                MSMICONS_FOLDER " Open .osim's parent directory",
                std::nullopt,
                false,
                model_has_backing_file) &&
            maybe_model) {

            action_open_osim_parent_directory(*maybe_model);
        }

        if (ui::draw_menu_item(
                MSMICONS_LINK " Open .osim in external editor",
                std::nullopt,
                false,
                model_has_backing_file) &&
            maybe_model) {

            action_open_osim_in_external_editor(*maybe_model);
        }
        ui::draw_tooltip_if_item_hovered(
            "Open .osim in external editor",
            "Open the .osim file currently being edited in an external text editor. The editor that's used depends on your operating system's default for opening .osim files.");
    }

    // reload
    // copy path to clipboard
    // parent dir
    // external editor

    ui::draw_separator();

    if (ui::draw_menu_item(MSMICONS_FILE_IMPORT " Import Meshes")) {
        if (parent()) {
            auto tab = std::make_unique<MeshImporterTab>(parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
    }
    ui::add_screenshot_annotation_to_last_drawn_item("MainMenu/ImportMeshesMenuItem");
    if (ui::draw_menu_item(MSMICONS_BEZIER_CURVE " Preview Experimental Data")) {
        if (parent()) {
            auto tab = std::make_unique<PreviewExperimentalDataTab>(parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
    }

    ui::draw_separator();

    if (ui::draw_menu_item(MSMICONS_TIMES_CIRCLE " Quit", ModifierKey::Ctrl | Key::Q)) {
        App::upd().request_quit();
    }

    ui::end_menu();
}

void osc::MainMenuAboutTab::on_draw()
{
    if (not ui::begin_menu("About")) {
        return;
    }

    constexpr float menu_width = 400;
    ui::draw_dummy({menu_width, 0});

    ui::draw_text("graphics");
    ui::same_line();
    ui::draw_help_marker("OSMV's global graphical settings");
    ui::draw_separator();
    ui::draw_vertical_spacer(0.5f / 15.0f);
    {
        ui::set_num_columns(2);

        ui::draw_text("FPS");
        ui::next_column();
        ui::draw_text("{:.0f}", ui::get_framerate());
        ui::next_column();

        ui::draw_text("MSXAA");
        ui::same_line();
        ui::draw_help_marker(
            "the log_level_ of MultiSample Anti-Aliasing to use. This only affects 3D renders *within* the UI, not the whole UI (panels etc. will not be affected)");
        ui::next_column();
        {
            const AntiAliasingLevel current = App::get().anti_aliasing_level();
            const AntiAliasingLevel max = App::get().max_anti_aliasing_level();

            if (ui::begin_combobox("##msxaa", stream_to_string(current))) {
                for (AntiAliasingLevel l = AntiAliasingLevel::min(); l <= max; ++l) {
                    bool selected = l == current;
                    if (ui::draw_selectable(stream_to_string(l), &selected)) {
                        App::upd().set_anti_aliasing_level(l);
                    }
                }
                ui::end_combobox();
            }
        }
        ui::next_column();

        ui::draw_text("window");
        ui::next_column();

        if (ui::draw_button(MSMICONS_EXPAND " fullscreen")) {
            App::upd().make_main_window_fullscreen();
        }
        if (ui::draw_button(MSMICONS_WINDOW_RESTORE " windowed")) {
            App::upd().make_main_window_windowed();
        }
        ui::next_column();

        ui::draw_text("VSYNC");
        ui::same_line();
        ui::draw_help_marker(
            "whether the backend uses vertical sync (VSYNC), which will cap the rendering FPS to your monitor's refresh rate");
        ui::next_column();

        bool enabled = App::get().vsync_enabled();
        if (ui::draw_checkbox("##vsynccheckbox", &enabled)) {
            App::upd().set_vsync_enabled(enabled);
        }
        ui::next_column();

        ui::set_num_columns();
    }

    ui::draw_vertical_spacer(2.0f / 15.0f);
    ui::draw_text("properties");
    ui::same_line();
    ui::draw_help_marker("general software properties: useful information for bug reporting etc.");
    ui::draw_separator();
    ui::draw_vertical_spacer(0.5f / 15.0f);
    {
        const AppMetadata& metadata = App::get().metadata();

        ui::set_num_columns(2);

        ui::draw_text("VERSION");
        ui::next_column();
        ui::draw_text(metadata.version_string().value_or("(not known)"));
        ui::next_column();

        if (const auto build_id = metadata.build_id()) {
            ui::draw_text("BUILD_ID");
            ui::next_column();
            ui::draw_text(*build_id);
            ui::next_column();
        }

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

    ui::draw_vertical_spacer(2.5f / 15.0f);
    ui::draw_text("debugging utilities:");
    ui::same_line();
    ui::draw_help_marker("standard utilities that can help with development, debugging, etc.");
    ui::draw_separator();
    ui::draw_vertical_spacer(0.5f / 15.0f);
    int id = 0;
    {
        ui::set_num_columns(2);

        if (const auto executable_directory = App::get().executable_directory()) {
            ui::draw_text("OSC Install Location");
            ui::same_line();
            ui::draw_help_marker(
                "opens OSC's installation location in your OS's default file browser");
            ui::next_column();
            ui::push_id(id++);

            if (ui::draw_button(MSMICONS_FOLDER " open")) {
                open_file_in_os_default_application(*executable_directory);
            }
            ui::pop_id();
            ui::next_column();
        }

        ui::draw_text("User Data Dir");
        ui::same_line();
        ui::draw_help_marker(
            "opens your OSC user data directory in your OS's default file browser");
        ui::next_column();
        ui::push_id(id++);
        if (ui::draw_button(MSMICONS_FOLDER " open")) {
            open_file_in_os_default_application(App::get().user_data_directory());
        }
        ui::pop_id();
        ui::next_column();

        ui::draw_text("Debug mode");
        ui::same_line();
        ui::draw_help_marker(
            "Toggles whether the application is in debug mode or not: enabling this can reveal more inforamtion about bugs");
        ui::next_column();
        {
            bool app_is_in_debug_mode = App::get().debug_mode();
            if (ui::draw_checkbox("##debugmodecheckbox", &app_is_in_debug_mode)) {
                App::upd().set_debug_mode(app_is_in_debug_mode);
            }
        }

        ui::set_num_columns();
    }

    ui::draw_vertical_spacer(2.5f / 15.0f);
    ui::draw_text("useful links:");
    ui::same_line();
    ui::draw_help_marker("links to external sites that might be useful");
    ui::draw_separator();
    ui::draw_vertical_spacer(0.5f / 15.0f);
    {
        ui::set_num_columns(2);

        if (auto docs_url = App::get().metadata().documentation_url()) {
            ui::draw_text("OpenSim Creator Documentation");
            ui::next_column();
            ui::push_id(id++);
            if (ui::draw_button(MSMICONS_LINK " open")) {
                open_url_in_os_default_web_browser(*docs_url);
            }
            ui::draw_tooltip_body_only_if_item_hovered(
                "this will open the documentation in a separate browser window");
            ui::pop_id();
            ui::next_column();
        }

        if (auto repo_url = App::get().metadata().repository_url()) {
            ui::draw_text("OpenSim Creator Repository");
            ui::next_column();
            ui::push_id(id++);
            if (ui::draw_button(MSMICONS_LINK " open")) {
                open_url_in_os_default_web_browser(*repo_url);
            }
            ui::draw_tooltip_body_only_if_item_hovered(
                "this will open the repository homepage in a separate browser window");
            ui::pop_id();
            ui::next_column();
        }

        if (auto help_url = App::get().metadata().help_url()) {
            ui::draw_text("OpenSim Creator Help");
            ui::next_column();
            ui::push_id(id++);
            if (ui::draw_button(MSMICONS_LINK " open")) {
                open_url_in_os_default_web_browser(*help_url);
            }
            ui::draw_tooltip_body_only_if_item_hovered(
                "this will open the help/discussion page in a separate browser window");
            ui::pop_id();
            ui::next_column();
        }

        ui::draw_text("OpenSim Documentation");
        ui::next_column();
        ui::push_id(id++);
        if (ui::draw_button(MSMICONS_LINK " open")) {
            open_url_in_os_default_web_browser(
                "https://simtk-confluence.stanford.edu/display/OpenSim/Documentation");
        }
        ui::draw_tooltip_body_only_if_item_hovered(
            "this will open the documentation in a separate browser window");
        ui::pop_id();
        ui::next_column();

        ui::set_num_columns();
    }

    ui::end_menu();
}
