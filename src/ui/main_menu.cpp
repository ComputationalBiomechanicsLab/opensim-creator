#include "main_menu.hpp"

#include "osc_build_config.hpp"
#include "src/application.hpp"
#include "src/styling.hpp"
#include "src/config.hpp"
#include "src/log.hpp"
#include "src/screens/imgui_demo_screen.hpp"
#include "src/screens/loading_screen.hpp"
#include "src/screens/model_editor_screen.hpp"
#include "src/screens/experiments_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/ui/help_marker.hpp"
#include "src/utils/helpers.hpp"
#include "src/utils/scope_guard.hpp"
#include "src/main_editor_state.hpp"
#include "src/ui/component_3d_viewer.hpp"
#include "src/utils/os.hpp"

#include <GL/glew.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>
#include <nfd.h>

#include <array>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>

using namespace osc;

namespace {
    void do_open_file_via_dialog(std::shared_ptr<Main_editor_state> st) {
        nfdchar_t* outpath = nullptr;

        nfdresult_t result = NFD_OpenDialog("osim", nullptr, &outpath);
        OSC_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

        if (result == NFD_OKAY) {
            Application::current().request_transition<Loading_screen>(st, outpath);
        }
    }

    std::optional<std::filesystem::path> prompt_save_single_file() {
        nfdchar_t* outpath = nullptr;
        nfdresult_t result = NFD_SaveDialog("osim", nullptr, &outpath);
        OSC_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

        return result == NFD_OKAY ? std::optional{std::string{outpath}} : std::nullopt;
    }

    bool is_subpath(std::filesystem::path const& dir, std::filesystem::path const& pth) {
        auto dir_n = std::distance(dir.begin(), dir.end());
        auto pth_n = std::distance(pth.begin(), pth.end());

        if (pth_n < dir_n) {
            return false;
        }

        return std::equal(dir.begin(), dir.end(), pth.begin());
    }

    bool is_example_file(std::filesystem::path const& path) {
        return is_subpath(resource("models"), path);
    }

    // fmap an optional from T -> f(T)
    template<typename T, typename MappingFunction>
    static auto map_optional(MappingFunction f, std::optional<T> opt)
        -> std::optional<decltype(f(std::move(opt).value()))> {

        return opt ? std::optional{f(std::move(opt).value())} : std::optional<decltype(f(std::move(opt).value()))>{};
    }

    std::string path2string(std::filesystem::path p) {
        return p.string();
    }

    std::optional<std::string> try_get_save_location(OpenSim::Model const& m) {

        if (std::string const& backing_path = m.getInputFileName();
            backing_path != "Unassigned" && backing_path.size() > 0) {

            // the model has an associated file
            //
            // we can save over this document - *IF* it's not an example file
            if (is_example_file(backing_path)) {
                return map_optional(path2string, prompt_save_single_file());
            } else {
                return backing_path;
            }
        } else {
            // the model has no associated file, so prompt the user for a save
            // location
            return map_optional(path2string, prompt_save_single_file());
        }
    }

    void save_model(OpenSim::Model& model, std::string const& save_loc) {
        try {
            model.print(save_loc);
            model.setInputFileName(save_loc);
            log::info("saved model to %s", save_loc.c_str());
            add_recent_file(save_loc);
        } catch (OpenSim::Exception const& ex) {
            log::error("error saving model: %s", ex.what());
        }
    }

    void transition_to_loading_existing_path(std::shared_ptr<Main_editor_state> st, std::filesystem::path p) {
        Application::current().request_transition<Loading_screen>(st, p);
    }

    std::unique_ptr<Component_3d_viewer> create_3dviewer() {
        return std::make_unique<Component_3d_viewer>(Component3DViewerFlags_Default | Component3DViewerFlags_DrawFrames);
    }
}


// public API

osc::ui::main_menu::file_tab::State::State() :
    example_osims{find_files_with_extensions(resource("models"), ".osim")},
    recent_files{osc::recent_files()} {

    std::sort(example_osims.begin(), example_osims.end(), filename_lexographically_gt);
}

void osc::ui::main_menu::about_tab::draw() {
    if (!ImGui::BeginMenu("About")) {
        return;
    }

    static constexpr float menu_width = 400;
    ImGui::Dummy(ImVec2(menu_width, 0));

    ImGui::TextUnformatted("graphics");
    ImGui::SameLine();
    ui::help_marker::draw("OSMV's global graphical settings");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 0.5f));
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("FPS");
        ImGui::NextColumn();
        ImGui::Text("%.0f", static_cast<double>(ImGui::GetIO().Framerate));
        ImGui::NextColumn();

        ImGui::TextUnformatted("MSXAA");
        ImGui::SameLine();
        ui::help_marker::draw("the level of MultiSample Anti-Aliasing to use. This only affects 3D renders *within* the UI, not the whole UI (panels etc. will not be affected)");
        ImGui::NextColumn();
        {
            static constexpr std::array<char const*, 8> aa_lvls = {"x1", "x2", "x4", "x8", "x16", "x32", "x64", "x128"};
            int samples_idx = lsb_index(Application::current().samples());
            int max_samples_idx = lsb_index(Application::current().max_samples());
            OSC_ASSERT(static_cast<size_t>(max_samples_idx) < aa_lvls.size());

            if (ImGui::Combo("##msxaa", &samples_idx, aa_lvls.data(), max_samples_idx + 1)) {
                Application::current().set_samples(1 << samples_idx);
            }
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("window");
        ImGui::NextColumn();

        if (ImGui::Button(ICON_FA_EXPAND " fullscreen")) {
            Application::current().make_fullscreen();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_WINDOW_RESTORE " windowed")) {
            Application::current().make_windowed();
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("VSYNC");
        ImGui::SameLine();
        ui::help_marker::draw("whether the backend uses vertical sync (VSYNC), which will cap the rendering FPS to your monitor's refresh rate");
        ImGui::NextColumn();

        if (Application::current().is_vsync_enabled()) {
            if (ImGui::Button("disable")) {
                Application::current().disable_vsync();
            }
        } else {
            if (ImGui::Button("enable")) {
                Application::current().enable_vsync();
            }
        }
        ImGui::NextColumn();

        ImGui::Columns();
    }

    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::TextUnformatted("properties");
    ImGui::SameLine();
    ui::help_marker::draw("general software properties: useful information for bug reporting etc.");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 0.5f));
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("OSC_VERSION");
        ImGui::NextColumn();
        ImGui::TextUnformatted(OSC_VERSION_STRING);
        ImGui::NextColumn();

        ImGui::TextUnformatted("OSC_BUILD_ID");
        ImGui::NextColumn();
        ImGui::TextUnformatted(OSC_BUILD_ID);
        ImGui::NextColumn();

        ImGui::TextUnformatted("GL_VENDOR");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_VENDOR));
        ImGui::NextColumn();

        ImGui::TextUnformatted("GL_RENDERER");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_RENDERER));
        ImGui::NextColumn();

        ImGui::TextUnformatted("GL_VERSION");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_VERSION));
        ImGui::NextColumn();

        ImGui::TextUnformatted("GL_SHADING_LANGUAGE_VERSION");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_SHADING_LANGUAGE_VERSION));
        ImGui::NextColumn();

        ImGui::Columns(1);
    }

    ImGui::Dummy(ImVec2(0.0f, 2.5f));
    ImGui::TextUnformatted("debugging utilities");
    ImGui::SameLine();
    ui::help_marker::draw("standard utilities that can help with development, debugging, etc.");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 0.5f));
    int id = 0;
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("ImGui demo");
        ImGui::SameLine();
        ui::help_marker::draw(
            "shows the standard ImGui demo screen (ImGui::ShowDemoWindow). Useful for finding an ImGui feature.");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_EYE " show")) {
            Application::current().request_transition<Imgui_demo_screen>();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::TextUnformatted("Experimental Screens");
        ImGui::SameLine();
        ui::help_marker::draw(
            "opens a test screen for experimental features - you probably don't care about this, but it's useful for testing hardware features in prod");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_EYE " show")) {
            Application::current().request_transition<Experiments_screen>();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::TextUnformatted("Debug mode");
        ImGui::SameLine();
        ui::help_marker::draw(
            "Toggles whether the application is in debug mode or not: enabling this can reveal more inforamtion about bugs");
        ImGui::NextColumn();
        {
            Application& app = Application::current();
            bool debug_mode = app.is_in_debug_mode();
            if (ImGui::Checkbox("##opengldebugmodecheckbox", &debug_mode)) {
                if (debug_mode) {
                    app.enable_debug_mode();
                } else {
                    app.disable_debug_mode();
                }
            }
        }

        ImGui::Columns();
    }

    ImGui::Dummy(ImVec2(0.0f, 2.5f));
    ImGui::TextUnformatted("useful links:");
    ImGui::SameLine();
    ui::help_marker::draw("links to external sites that might be useful");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 0.5f));
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("OpenSim Creator GitHub");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_LINK " open")) {
            open_path_in_default_application(OSC_REPO_URL);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("this will open the GitHub homepage in a separate browser window");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::TextUnformatted("OpenSim Documentation");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_LINK " open")) {
            open_path_in_default_application("https://simtk-confluence.stanford.edu/display/OpenSim/Documentation");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("this will open the documentation in a separate browser window");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::Columns();
    }

    ImGui::EndMenu();
}

void osc::ui::main_menu::action_new_model(std::shared_ptr<Main_editor_state> mes) {
    OSC_ASSERT(mes && "editor state should be set");

    mes->tabs.emplace_back(std::make_unique<Undoable_ui_model>(std::make_unique<OpenSim::Model>()));
    mes->cur_tab = static_cast<int>(mes->tabs.size()) - 1;
    Application::current().request_transition<Model_editor_screen>(mes);
}

void osc::ui::main_menu::action_open_model(std::shared_ptr<Main_editor_state> mes) {
    OSC_ASSERT(mes && "editor state should be set");
    do_open_file_via_dialog(mes);
}

void osc::ui::main_menu::action_save(OpenSim::Model& model) {
    std::optional<std::string> maybe_save_loc = try_get_save_location(model);

    if (maybe_save_loc) {
        save_model(model, *maybe_save_loc);
    }
}

void osc::ui::main_menu::action_save_as(OpenSim::Model& model) {
    std::optional<std::string> maybe_path = map_optional(path2string, prompt_save_single_file());

    if (maybe_path) {
        save_model(model, *maybe_path);
    }
}

void osc::ui::main_menu::file_tab::draw(State& st, std::shared_ptr<Main_editor_state> mes) {
    OSC_ASSERT(mes && "editor state should be set");

    // handle hotkeys enabled by just drawing the menu
    { 
        auto const& io = ImGui::GetIO();
        bool mod = io.KeyCtrl || io.KeySuper;

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_N)) {
            action_new_model(mes);
        }

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_O)) {
            action_open_model(mes);
        }

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_S) && mes->focused_editor()) {
            action_save(mes->focused_editor()->model());
        }

        if (mod && io.KeyAlt && ImGui::IsKeyPressed(SDL_SCANCODE_S) && mes->focused_editor()) {
            action_save_as(mes->focused_editor()->model());
        }

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_W)) {
            Application::current().request_transition<Splash_screen>();
        }

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_Q)) {
            Application::current().request_quit();
        }
    }

    if (!ImGui::BeginMenu("File")) {
        return;
    }

    if (ImGui::MenuItem(ICON_FA_FILE " New", "Ctrl+N")) {
        action_new_model(mes);
    }

    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open", "Ctrl+O")) {
        action_open_model(mes);
    }

    int imgui_id = 0;

    if (ImGui::BeginMenu(ICON_FA_FOLDER_OPEN " Open Recent", !st.recent_files.empty())) {
        // iterate in reverse: recent files are stored oldest --> newest
        for (auto it = st.recent_files.rbegin(); it != st.recent_files.rend(); ++it) {
            Recent_file const& rf = *it;
            ImGui::PushID(++imgui_id);
            if (ImGui::MenuItem(rf.path.filename().string().c_str())) {
                transition_to_loading_existing_path(mes, rf.path);
            }
            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(ICON_FA_FOLDER_OPEN " Open Example")) {
        for (std::filesystem::path const& ex : st.example_osims) {
            ImGui::PushID(++imgui_id);
            if (ImGui::MenuItem(ex.filename().string().c_str())) {
                transition_to_loading_existing_path(mes, ex);
            }
            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if (ImGui::MenuItem(ICON_FA_SAVE " Save", "Ctrl+S", false, mes->focused_editor())) {
        action_save(mes->focused_editor()->model());
    }

    if (ImGui::MenuItem(ICON_FA_SAVE " Save As", "Shift+Ctrl+S", false, mes->focused_editor())) {
        action_save_as(mes->focused_editor()->model());
    }

    if (ImGui::MenuItem(ICON_FA_TIMES " Close", "Ctrl+W", false, mes != nullptr)) {
        Application::current().request_transition<Splash_screen>();
    }

    if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q")) {
        Application::current().request_quit();
    }
    ImGui::EndMenu();
}

void osc::ui::main_menu::window_tab::draw(Main_editor_state& st) {

    // draw "window" tab
    if (ImGui::BeginMenu("Window")) {

        ImGui::MenuItem("Actions", nullptr, &st.shown_panels.actions);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when editing a model");
            ImGui::EndTooltip();
        }

        ImGui::MenuItem("Hierarchy", nullptr, &st.shown_panels.hierarchy);

        ImGui::MenuItem("Log", nullptr, &st.shown_panels.log);

        ImGui::MenuItem("Outputs", nullptr, &st.shown_panels.outputs);

        ImGui::MenuItem("Property Editor", nullptr, &st.shown_panels.property_editor);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when editing a model");
            ImGui::EndTooltip();
        }

        ImGui::MenuItem("Selection Details", nullptr, &st.shown_panels.selection_details);

        ImGui::MenuItem("Simulations", nullptr, &st.shown_panels.simulations);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when simulating a model");
            ImGui::EndTooltip();
        }

        ImGui::MenuItem("Simulation Stats", nullptr, &st.shown_panels.simulation_stats);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when editing a model");
            ImGui::EndTooltip();
        }

        for (size_t i = 0; i < st.viewers.size(); ++i) {
            Component_3d_viewer* viewer = st.viewers[i].get();

            char buf[64];
            std::snprintf(buf, sizeof(buf), "viewer%zu", i);

            bool enabled = viewer != nullptr;
            if (ImGui::MenuItem(buf, nullptr, &enabled)) {
                if (enabled) {
                    // was enabled by user click
                    st.viewers[i] = create_3dviewer();
                } else {
                    // was disabled by user click
                    st.viewers[i] = nullptr;
                }
            }
        }

        ImGui::EndMenu();
    }
}
