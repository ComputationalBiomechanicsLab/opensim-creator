#include "attach_geometry_popup.hpp"

#include "src/resources.hpp"
#include "src/utils/scope_guard.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <imgui.h>
#include <nfd.h>

#include <cstddef>
#include <memory>
#include <string>
#include <optional>

namespace fs = std::filesystem;

static bool filename_lexographically_gt(fs::path const& a, fs::path const& b) {
    return a.filename() < b.filename();
}

std::vector<std::filesystem::path> osc::find_all_vtp_resources() {
    fs::path geometry_dir = osc::resource("geometry");
    std::vector<fs::path> rv = find_files_with_extensions(geometry_dir, ".vtp");
    std::sort(rv.begin(), rv.end(), filename_lexographically_gt);

    return rv;
}

static std::unique_ptr<OpenSim::Mesh>
    on_vtp_choice_made(osc::ui::attach_geometry_popup::State& st, std::filesystem::path path) {

    auto rv = std::make_unique<OpenSim::Mesh>(path.string());

    // add to recent list
    st.recent_user_choices.push_back(std::move(path));

    // reset search (for next popup open)
    st.search[0] = '\0';

    ImGui::CloseCurrentPopup();

    return rv;
}

static std::unique_ptr<OpenSim::Mesh>
    try_draw_file_choice(osc::ui::attach_geometry_popup::State& st, std::filesystem::path const& p) {

    if (p.filename().string().find(st.search.data()) != std::string::npos) {
        if (ImGui::Selectable(p.filename().string().c_str())) {
            return on_vtp_choice_made(st, p.filename());
        }
    }

    return nullptr;
}

static std::optional<std::filesystem::path> prompt_open_vtp() {
    nfdchar_t* outpath = nullptr;
    nfdresult_t result = NFD_OpenDialog("vtp", nullptr, &outpath);
    OSC_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

    return result == NFD_OKAY ? std::optional{std::string{outpath}} : std::nullopt;
}

std::unique_ptr<OpenSim::Mesh> osc::ui::attach_geometry_popup::draw(State& st, char const* modal_name) {

    // center the modal
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // try to show the modal (depends on caller calling ImGui::OpenPopup)
    if (!ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return nullptr;
    }

    // let user type a search
    ImGui::InputText("search", st.search.data(), st.search.size());
    ImGui::Dummy(ImVec2{0.0f, 1.0f});

    std::unique_ptr<OpenSim::Mesh> rv = nullptr;

    // show previous (recent) user choices
    if (!st.recent_user_choices.empty()) {
        ImGui::Text("recent:");
        ImGui::BeginChild(
            "recent meshes", ImVec2(ImGui::GetContentRegionAvail().x, 64), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (std::filesystem::path const& p : st.recent_user_choices) {
            auto resp = try_draw_file_choice(st, p);
            if (resp) {
                rv = std::move(resp);
            }
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
            auto resp = try_draw_file_choice(st, p);
            if (resp) {
                rv = std::move(resp);
            }
        }

        ImGui::EndChild();
    }

    if (ImGui::Button("Open")) {
        if (auto maybe_vtp = prompt_open_vtp(); maybe_vtp) {
            rv = on_vtp_choice_made(st, std::move(maybe_vtp).value());
        }
    }

    if (ImGui::Button("Cancel")) {
        st.search[0] = '\0';
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();

    return rv;
}
