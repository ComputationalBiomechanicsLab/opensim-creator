#include "main_menu.hpp"

#include "osc_build_config.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/log.hpp"
#include "src/screens/imgui_demo_screen.hpp"
#include "src/screens/loading_screen.hpp"
#include "src/screens/model_editor_screen.hpp"
#include "src/screens/opengl_test_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/ui/help_marker.hpp"
#include "src/utils/bitwise_algs.hpp"
#include "src/utils/scope_guard.hpp"

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

static void draw_header(char const* str) {
    ImGui::TextUnformatted(str);
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 0.5f));
}

void osc::ui::main_menu::about_tab::draw() {
    if (!ImGui::BeginMenu("About")) {
        return;
    }

    static constexpr float menu_width = 400;
    ImGui::Dummy(ImVec2(menu_width, 0));

    draw_header("graphics");
    {
        ImGui::Columns(2);

        ImGui::Text("FPS");
        ImGui::NextColumn();
        ImGui::Text("%.1f", static_cast<double>(ImGui::GetIO().Framerate));
        ImGui::NextColumn();

        ImGui::Text("MSXAA");
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

        ImGui::Text("window");
        ImGui::NextColumn();

        if (ImGui::Button("fullscreen")) {
            Application::current().make_fullscreen();
        }
        ImGui::SameLine();
        if (ImGui::Button("windowed")) {
            Application::current().make_windowed();
        }
        ImGui::NextColumn();

        ImGui::Text("VSYNC");
        ImGui::NextColumn();
        if (ImGui::Button("enable")) {
            Application::current().enable_vsync();
        }
        ImGui::SameLine();
        if (ImGui::Button("disable")) {
            Application::current().disable_vsync();
        }
        ImGui::NextColumn();

        ImGui::Columns();
    }

    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    draw_header("properties");
    {
        ImGui::Columns(2);

        ImGui::Text("OSC_VERSION");
        ImGui::NextColumn();
        ImGui::Text("%s", OSC_VERSION_STRING);
        ImGui::NextColumn();

        ImGui::Text("OSC_BUILD_ID");
        ImGui::NextColumn();
        ImGui::Text("%s", OSC_BUILD_ID);
        ImGui::NextColumn();

        ImGui::Text("GL_VENDOR");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_VENDOR));
        ImGui::NextColumn();

        ImGui::Text("GL_RENDERER");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_RENDERER));
        ImGui::NextColumn();

        ImGui::Text("GL_VERSION");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_VERSION));
        ImGui::NextColumn();

        ImGui::Text("GL_SHADING_LANGUAGE_VERSION");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_SHADING_LANGUAGE_VERSION));
        ImGui::NextColumn();

        ImGui::Columns(1);
    }

    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    draw_header("utils");
    {
        ImGui::Columns(2);

        ImGui::Text("ImGui demo");
        ImGui::SameLine();
        draw_help_marker(
            "shows the standard ImGui demo screen (ImGui::ShowDemoWindow). Useful for finding an ImGui feature.");
        ImGui::NextColumn();
        int id = 0;
        ImGui::PushID(id++);
        if (ImGui::Button("show")) {
            Application::current().request_screen_transition<Imgui_demo_screen>();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::Text("OpenGL experiments");
        ImGui::SameLine();
        draw_help_marker(
            "opens a test screen for low-level OpenGL features - you probably don't care about this, but it's useful for testing hardware features in prod");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button("show")) {
            Application::current().request_screen_transition<Opengl_test_screen>();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::Text("Debug mode");
        ImGui::SameLine();
        draw_help_marker(
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

    ImGui::EndMenu();
}

static void do_open_file_via_dialog() {
    nfdchar_t* outpath = nullptr;

    nfdresult_t result = NFD_OpenDialog("osim", nullptr, &outpath);
    OSC_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

    if (result == NFD_OKAY) {
        Application::current().request_screen_transition<Loading_screen>(outpath);
    }
}

static std::optional<std::filesystem::path> prompt_save_single_file() {
    nfdchar_t* outpath = nullptr;
    nfdresult_t result = NFD_SaveDialog("osim", nullptr, &outpath);
    OSC_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

    return result == NFD_OKAY ? std::optional{std::string{outpath}} : std::nullopt;
}

static bool is_subpath(std::filesystem::path const& dir, std::filesystem::path const& pth) {
    auto dir_n = std::distance(dir.begin(), dir.end());
    auto pth_n = std::distance(pth.begin(), pth.end());

    if (pth_n < dir_n) {
        return false;
    }

    return std::equal(dir.begin(), dir.end(), pth.begin());
}

static bool is_example_file(std::filesystem::path const& path) {
    std::filesystem::path examples_dir = config::resource_path("models");
    return is_subpath(examples_dir, path);
}

template<typename T, typename MappingFunction>
static auto map_optional(MappingFunction f, std::optional<T> opt)
    -> std::optional<decltype(f(std::move(opt).value()))> {

    return opt ? std::optional{f(std::move(opt).value())} : std::optional<decltype(f(std::move(opt).value()))>{};
}

static std::string path2string(std::filesystem::path p) {
    return p.string();
}

static std::optional<std::string> try_get_save_location(OpenSim::Model const& m) {

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

static void save_model(OpenSim::Model& model, std::string const& save_loc) {
    try {
        model.print(save_loc);
        model.setInputFileName(save_loc);
        log::info("saved model to %s", save_loc.c_str());
        config::add_recent_file(save_loc);
    } catch (OpenSim::Exception const& ex) {
        log::error("error saving model: %s", ex.what());
    }
}

void osc::ui::main_menu::action_new_model() {
    Application::current().request_screen_transition<Model_editor_screen>();
}

void osc::ui::main_menu::action_open_model() {
    do_open_file_via_dialog();
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

void osc::ui::main_menu::file_tab::draw(State& st, OpenSim::Model* opened_model) {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New", "Ctrl+N")) {
            action_new_model();
        }

        if (ImGui::MenuItem("Open", "Ctrl+O")) {
            action_open_model();
        }

        int id = 0;

        if (ImGui::BeginMenu("Open Recent")) {
            // iterate in reverse: recent files are stored oldest --> newest
            for (auto it = st.recent_files.rbegin(); it != st.recent_files.rend(); ++it) {
                config::Recent_file const& rf = *it;
                ImGui::PushID(++id);
                if (ImGui::MenuItem(rf.path.filename().string().c_str())) {
                    Application::current().request_screen_transition<Loading_screen>(rf.path);
                }
                ImGui::PopID();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Open Example")) {
            for (std::filesystem::path const& ex : st.example_osims) {
                ImGui::PushID(++id);
                if (ImGui::MenuItem(ex.filename().string().c_str())) {
                    Application::current().request_screen_transition<Loading_screen>(ex);
                }
                ImGui::PopID();
            }

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Save", "Ctrl+S", false, opened_model)) {
            if (opened_model) {
                action_save(*opened_model);
            }
        }

        if (ImGui::MenuItem("Save As", "Shift+Ctrl+S", false, opened_model)) {
            if (opened_model) {
                action_save_as(*opened_model);
            }
        }

        if (ImGui::MenuItem("Show Splash Screen")) {
            Application::current().request_screen_transition<Splash_screen>();
        }

        if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
            Application::current().request_quit_application();
        }
        ImGui::EndMenu();
    }
}
