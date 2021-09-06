#include "AttachGeometryPopup.hpp"

#include "src/ui/help_marker.hpp"
#include "src/utils/ScopeGuard.hpp"
#include "src/utils/FilesystemHelpers.hpp"
#include "src/App.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <imgui.h>
#include <nfd.h>

#include <cstddef>
#include <memory>
#include <string>
#include <optional>

namespace fs = std::filesystem;
using namespace osc;

namespace {
    using Geom_ctor_fn = std::unique_ptr<OpenSim::Geometry>(*)(void);
    constexpr std::array<Geom_ctor_fn const, 7> g_GeomCtors = {
        []() {
            auto ptr = std::make_unique<OpenSim::Brick>();
            ptr->set_half_lengths(SimTK::Vec3{0.1, 0.1, 0.1});
            return std::unique_ptr<OpenSim::Geometry>(std::move(ptr));
        },
        []() {
            auto ptr = std::make_unique<OpenSim::Sphere>();
            ptr->set_radius(0.1);
            return std::unique_ptr<OpenSim::Geometry>{std::move(ptr)};
        },
        []() {
            auto ptr = std::make_unique<OpenSim::Cylinder>();
            ptr->set_radius(0.1);
            ptr->set_half_height(0.1);
            return std::unique_ptr<OpenSim::Geometry>{std::move(ptr)};
        },
        []() { return std::unique_ptr<OpenSim::Geometry>{new OpenSim::LineGeometry{}}; },
        []() { return std::unique_ptr<OpenSim::Geometry>{new OpenSim::Ellipsoid{}}; },
        []() { return std::unique_ptr<OpenSim::Geometry>{new OpenSim::Arrow{}}; },
        []() { return std::unique_ptr<OpenSim::Geometry>{new OpenSim::Cone{}}; },
    };
    constexpr std::array<char const* const, 7> g_GeomNames = {
        "Brick",
        "Sphere",
        "Cylinder",
        "LineGeometry",
        "Ellipsoid",
        "Arrow (CARE: may not work in OpenSim's main UI)",
        "Cone",
    };
    static_assert(g_GeomCtors.size() == g_GeomNames.size());

    std::unique_ptr<OpenSim::Mesh> onVTPChoiceMade(AttachGeometryPopup& st, std::filesystem::path path) {

        auto rv = std::make_unique<OpenSim::Mesh>(path.string());

        // add to recent list
        st.recentUserChoices.push_back(std::move(path));

        // reset search (for next popup open)
        st.search[0] = '\0';

        ImGui::CloseCurrentPopup();

        return rv;
    }

    std::unique_ptr<OpenSim::Mesh> tryDrawFileChoice(AttachGeometryPopup& st, std::filesystem::path const& p) {
        if (p.filename().string().find(st.search.data()) != std::string::npos) {
            if (ImGui::Selectable(p.filename().string().c_str())) {
                return onVTPChoiceMade(st, p.filename());
            }
        }
        return nullptr;
    }

    std::optional<std::filesystem::path> promptOpenVTP() {
        nfdchar_t* outpath = nullptr;
        nfdresult_t result = NFD_OpenDialog("vtp", nullptr, &outpath);
        OSC_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

        return result == NFD_OKAY ? std::optional{std::string{outpath}} : std::nullopt;
    }
}

osc::AttachGeometryPopup::AttachGeometryPopup() :
    vtps{GetAllFilesInDirRecursively(App::resource("geometry"))},
    recentUserChoices{},
    search{} {
}

std::unique_ptr<OpenSim::Geometry> osc::AttachGeometryPopup::draw(char const* modalName) {
    auto& st = *this;

    std::unique_ptr<OpenSim::Geometry> rv = nullptr;

    // center the modal
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // try to show the modal (depends on caller calling ImGui::OpenPopup)
    if (!ImGui::BeginPopupModal(modalName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return rv;
    }

    // premade geometry section
    //
    // let user select from a shorter sequence of analytical geometry that can be
    // generated without a mesh file
    {
        ImGui::TextUnformatted("Generated geometry");
        ImGui::SameLine();
        ui::help_marker::draw("This is geometry that OpenSim can generate without needing an external mesh file. Useful for basic geometry.");
        ImGui::Separator();
        ImGui::Dummy(ImVec2{0.0f, 2.0f});

        int item = -1;
        if (ImGui::Combo("##premade", &item, g_GeomNames.data(), static_cast<int>(g_GeomNames.size()))) {
            auto const& ctor = g_GeomCtors[static_cast<size_t>(item)];
            rv = ctor();
            st.search[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
    }

    // mesh file selection
    //
    // let the user select a mesh file that the implementation should load + use
    ImGui::Dummy(ImVec2{0.0f, 3.0f});
    ImGui::TextUnformatted("mesh file");
    ImGui::SameLine();
    ui::help_marker::draw("This is geometry that OpenSim loads from external mesh files. Useful for custom geometry (usually, created in some other application, such as ParaView or Blender)");
    ImGui::Separator();
    ImGui::Dummy(ImVec2{0.0f, 2.0f});

    // let the user search through mesh files in pre-established Geometry/ dirs
    ImGui::InputText("search", st.search.data(), st.search.size());
    ImGui::Dummy(ImVec2{0.0f, 1.0f});

    ImGui::BeginChild(
        "mesh list", ImVec2(ImGui::GetContentRegionAvail().x, 256), false, ImGuiWindowFlags_HorizontalScrollbar);


    if (!st.recentUserChoices.empty()) {
        ImGui::TextDisabled("  (recent)");
    }
    for (std::filesystem::path const& p : st.recentUserChoices) {
        auto resp = tryDrawFileChoice(st, p);
        if (resp) {
            rv = std::move(resp);
        }
    }

    if (!st.recentUserChoices.empty()) {
        ImGui::TextDisabled("  (from Geometry/ dir)");
    }
    for (std::filesystem::path const& p : st.vtps) {
        auto resp = tryDrawFileChoice(st, p);
        if (resp) {
            rv = std::move(resp);
        }
    }

    ImGui::EndChild();

    if (ImGui::Button("Open Mesh File")) {
        if (auto maybeVTP = promptOpenVTP(); maybeVTP) {
            rv = onVTPChoiceMade(st, std::move(maybeVTP).value());
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted("Open a mesh file on the filesystem");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    ImGui::Dummy(ImVec2{0.0f, 5.0f});

    if (ImGui::Button("Cancel")) {
        st.search[0] = '\0';
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();

    return rv;
}
