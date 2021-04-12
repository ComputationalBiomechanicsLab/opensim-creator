#include "attach_geometry_modal.hpp"

#include "src/config.hpp"
#include "src/utils/scope_guard.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <imgui.h>
#include <nfd.h>

#include <cstddef>
#include <memory>
#include <string>

namespace fs = std::filesystem;

static bool filename_lexographically_gt(fs::path const& a, fs::path const& b) {
    return a.filename() < b.filename();
}

std::vector<std::filesystem::path> osc::find_all_vtp_resources() {
    fs::path geometry_dir = osc::config::resource_path("geometry");

    std::vector<fs::path> rv;

    if (!fs::exists(geometry_dir)) {
        // application installation is probably mis-configured, or missing
        // the geometry dir (e.g. the user deleted it)
        return rv;
    }

    if (!fs::is_directory(geometry_dir)) {
        // something horrible has happened, such as the user creating a file
        // called "geometry" in the application resources dir. Silently eat
        // this for now
        return rv;
    }

    // ensure the number of files iterated over does not exeed some (arbitrary)
    // limit to protect the application from the edge-case that the implementation
    // selects (e.g.) a root directory and ends up recursing over the entire
    // filesystem
    int i = 0;
    static constexpr int file_limit = 10000;

    for (fs::directory_entry const& entry : fs::recursive_directory_iterator{geometry_dir}) {
        if (i++ > file_limit) {
            // TODO: log warning
            return rv;
        }

        if (entry.path().extension() == ".vtp") {
            rv.push_back(entry.path());
        }
    }

    std::sort(rv.begin(), rv.end(), filename_lexographically_gt);

    return rv;
}

static void on_vtp_choice_made(
    osc::Attach_geometry_modal_state& st,
    std::function<void(std::unique_ptr<OpenSim::Mesh>)> const& out,
    std::filesystem::path path) {

    // pass selected mesh to caller
    out(std::make_unique<OpenSim::Mesh>(path.string()));

    // add to recent list
    st.recent_user_choices.push_back(std::move(path));

    // reset search (for next popup open)
    st.search[0] = '\0';

    ImGui::CloseCurrentPopup();
}

static void try_draw_file_choice(
    osc::Attach_geometry_modal_state& st,
    std::filesystem::path const& p,
    std::function<void(std::unique_ptr<OpenSim::Mesh>)> const& out) {

    if (p.filename().string().find(st.search.data()) != std::string::npos) {
        if (ImGui::Selectable(p.filename().string().c_str())) {
            on_vtp_choice_made(st, out, p.filename());
        }
    }
}

static std::optional<std::filesystem::path> prompt_open_vtp() {
    nfdchar_t* outpath = nullptr;
    nfdresult_t result = NFD_OpenDialog("vtp", nullptr, &outpath);
    OSC_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

    return result == NFD_OKAY ? std::optional{std::string{outpath}} : std::nullopt;
}

void osc::draw_attach_geom_modal_if_opened(
    Attach_geometry_modal_state& st,
    char const* modal_name,
    std::function<void(std::unique_ptr<OpenSim::Mesh>)> const& out) {

    // center the modal
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // try to show the modal (depends on caller calling ImGui::OpenPopup)
    if (!ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    // let user type a search
    ImGui::InputText("search", st.search.data(), st.search.size());
    ImGui::Dummy(ImVec2{0.0f, 1.0f});

    // show previous (recent) user choices
    if (!st.recent_user_choices.empty()) {
        ImGui::Text("recent:");
        ImGui::BeginChild(
            "recent meshes", ImVec2(ImGui::GetContentRegionAvail().x, 64), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (std::filesystem::path const& p : st.recent_user_choices) {
            try_draw_file_choice(st, p, out);
        }

        ImGui::EndChild();
        ImGui::Dummy(ImVec2{0.0f, 1.0f});
    }

    // show list of VTPs (probably loaded from resource dir)
    {
        ImGui::Text("all:");
        ImGui::BeginChild(
            "all meshes", ImVec2(ImGui::GetContentRegionAvail().x, 256), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (std::filesystem::path const& p : st.vtps) {
            try_draw_file_choice(st, p, out);
        }

        ImGui::EndChild();
    }

    if (ImGui::Button("Open")) {
        if (auto maybe_vtp = prompt_open_vtp(); maybe_vtp) {
            on_vtp_choice_made(st, out, std::move(maybe_vtp).value());
        }
    }

    if (ImGui::Button("Cancel")) {
        st.search[0] = '\0';
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
