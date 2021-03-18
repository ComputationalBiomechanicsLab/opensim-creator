#include "main_menu_file_tab.hpp"

#include "src/application.hpp"
#include "src/config.hpp"
#include "src/log.hpp"
#include "src/screens/loading_screen.hpp"
#include "src/screens/model_editor_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/utils/scope_guard.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>
#include <nfd.h>

#include <filesystem>
#include <iterator>
#include <optional>
#include <string>

using namespace osmv;

static void do_open_file_via_dialog() {
    nfdchar_t* outpath = nullptr;

    nfdresult_t result = NFD_OpenDialog("osim", nullptr, &outpath);
    OSMV_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

    if (result == NFD_OKAY) {
        Application::current().request_screen_transition<Loading_screen>(outpath);
    }
}

static std::optional<std::filesystem::path> prompt_save_single_file() {
    nfdchar_t* outpath = nullptr;
    nfdresult_t result = NFD_SaveDialog("osim", nullptr, &outpath);
    OSMV_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

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
        backing_path != "Unassigned" and backing_path.size() > 0) {

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

void osmv::main_menu_new() {
    Application::current().request_screen_transition<Model_editor_screen>();
}

void osmv::main_menu_open() {
    do_open_file_via_dialog();
}

void osmv::main_menu_save(OpenSim::Model& model) {
    std::optional<std::string> maybe_save_loc = try_get_save_location(model);

    if (maybe_save_loc) {
        save_model(model, *maybe_save_loc);
    }
}

void osmv::main_menu_save_as(OpenSim::Model& model) {
    std::optional<std::string> maybe_path = map_optional(path2string, prompt_save_single_file());
    if (maybe_path) {
        save_model(model, *maybe_path);
    }
}

void osmv::draw_main_menu_file_tab(Main_menu_file_tab_state& st, OpenSim::Model* opened_model) {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New", "Ctrl+N")) {
            main_menu_new();
        }

        if (ImGui::MenuItem("Open", "Ctrl+O")) {
            main_menu_open();
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
                main_menu_save(*opened_model);
            }
        }

        if (ImGui::MenuItem("Save As", "Shift+Ctrl+S", false, opened_model)) {
            if (opened_model) {
                main_menu_save_as(*opened_model);
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
