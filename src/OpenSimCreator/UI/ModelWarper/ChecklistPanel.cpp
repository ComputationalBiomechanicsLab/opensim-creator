#include "ChecklistPanel.hpp"

#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Utils/Concepts.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using osc::BeginTooltip;
using osc::Color;
using osc::CStringView;
using osc::DerivedFrom;
using osc::DrawHelpMarker;
using osc::FindGeometryFileAbsPath;
using osc::GetAbsolutePathString;
using osc::EndTooltip;
using osc::PopStyleColor;
using osc::TextCentered;
using osc::TextUnformatted;
using osc::mow::MeshWarpPairing;
using osc::mow::UIState;

// data stuff
namespace
{
    struct InputCheck {
        enum class State { Ok, Warning, Error };

        InputCheck(
            std::string description_,
            bool passOrFail_) :

            description{std::move(description_)},
            state{passOrFail_ ? State::Ok : State::Error}
        {
        }

        InputCheck(
            std::string description_,
            State state_) :

            description{std::move(description_)},
            state{state_}
        {
        }

        std::string description;
        State state;
    };

    enum class SearchState { Continue, Stop };
    void ForEachCheck(
        MeshWarpPairing const& p,
        std::function<SearchState(InputCheck)> const& callback)
    {
        // has a source landmarks file
        {
            std::stringstream ss;
            ss << "has source landmarks file at " << p.recommendedSourceLandmarksFilepath().string();
            if (callback({ std::move(ss).str(), p.hasSourceLandmarksFilepath() }) == SearchState::Stop)
            {
                return;
            }
        }

        // has source landmarks
        {
            if (callback({ "source landmarks file contains landmarks", p.hasSourceLandmarks() }) == SearchState::Stop)
            {
                return;
            }
        }

        // has destination mesh file
        {
            std::stringstream ss;
            ss << "has destination mesh file at " << p.recommendedDestinationMeshFilepath().string();
            if (callback({ std::move(ss).str(), p.hasDestinationMeshFilepath() }) == SearchState::Stop)
            {
                return;
            }
        }

        // has destination landmarks file
        {
            std::stringstream ss;
            ss << "has destination landmarks file at " << p.recommendedDestinationLandmarksFilepath().string();
            if (callback({ std::move(ss).str(), p.hasDestinationLandmarksFilepath() }) == SearchState::Stop)
            {
                return;
            }
        }

        // has destination landmarks
        {
            if (callback({ "destination landmarks file contains landmarks", p.hasDestinationLandmarks() }) == SearchState::Stop)
            {
                return;
            }
        }

        // has at least a few paired landmarks
        {
            if (callback({ "at least three landmarks can be paired between source/destination", p.getNumFullyPairedLandmarks() >= 3 }) == SearchState::Stop)
            {
                return;
            }
        }

        // (warning): has no unpaired landmarks
        {
            if (callback({ "there are no unpaired landmarks", p.getNumUnpairedLandmarks() == 0 ? InputCheck::State::Ok : InputCheck::State::Warning }) == SearchState::Stop)
            {
                return;
            }
        }
    }

    void ForEachCheck(
        MeshWarpPairing const* p,
        std::function<SearchState(InputCheck)> const& callback)
    {
        if (p)
        {
            return ForEachCheck(*p, callback);
        }
        else
        {
            callback({ "no mesh warp pairing found: this is probably an implementation error (maybe reload?)", InputCheck::State::Error });
            return;
        }
    }

    InputCheck::State CalcWorstState(MeshWarpPairing const* p)
    {
        InputCheck::State worst = InputCheck::State::Ok;
        ForEachCheck(p, [&worst](InputCheck c)
        {
            if (c.state == InputCheck::State::Error)
            {
                worst = InputCheck::State::Error;
                return SearchState::Stop;
            }
            else if (c.state == InputCheck::State::Warning)
            {
                worst = InputCheck::State::Warning;
                return SearchState::Continue;
            }
            else
            {
                return SearchState::Continue;
            }
        });
        return worst;
    }

    struct PairingDetail final {
        std::string name;
        std::string value;
    };

    void ForEachDetailIn(
        MeshWarpPairing const& p,
        std::function<void(PairingDetail)> const& callback)
    {
        callback({ "source mesh filepath", p.getSourceMeshAbsoluteFilepath().string() });
        callback({ "source landmarks expected filepath", p.recommendedSourceLandmarksFilepath().string() });
        callback({ "has source landmarks file?", p.hasSourceLandmarksFilepath() ? "yes" : "no" });
        callback({ "number of source landmarks", std::to_string(p.getNumSourceLandmarks()) });
        callback({ "destination mesh expected filepath", p.recommendedDestinationMeshFilepath().string() });
        callback({ "has destination mesh?", p.hasDestinationMeshFilepath() ? "yes" : "no" });
        callback({ "destination landmarks expected filepath", p.recommendedDestinationLandmarksFilepath().string() });
        callback({ "has destination landmarks file?", p.hasDestinationLandmarksFilepath() ? "yes" : "no" });
        callback({ "number of destination landmarks", std::to_string(p.getNumDestinationLandmarks()) });
        callback({ "number of paired landmarks", std::to_string(p.getNumFullyPairedLandmarks()) });
        callback({ "number of unpaired landmarks", std::to_string(p.getNumUnpairedLandmarks()) });
    }

    void ForEachDetailIn(
        OpenSim::Mesh const& mesh,
        MeshWarpPairing const* maybePairing,
        std::function<void(PairingDetail)> const& callback)
    {
        callback({ "OpenSim::Mesh path in the OpenSim::Model", GetAbsolutePathString(mesh) });
        if (maybePairing)
        {
            ForEachDetailIn(*maybePairing, callback);
        }
    }
}

// UI (generic)
namespace
{
    struct EntryStyling final {
        CStringView icon;
        Color color;
    };

    EntryStyling ToStyle(InputCheck::State s)
    {
        switch (s)
        {
        case InputCheck::State::Ok:
            return {.icon = ICON_FA_CHECK, .color = Color::green()};
        case InputCheck::State::Warning:
            return {.icon = ICON_FA_EXCLAMATION, .color = Color::orange()};
        default:
        case InputCheck::State::Error:
            return {.icon = ICON_FA_TIMES, .color = Color::red()};
        }
    }

    EntryStyling CalcStyle(UIState const& state, OpenSim::Mesh const& mesh)
    {
        InputCheck::State const worst = CalcWorstState(state.findMeshWarp(mesh));
        return ToStyle(worst);
    }

    EntryStyling CalcStyle(UIState const&, OpenSim::Frame const&)
    {
        return {.icon = ICON_FA_TIMES, .color = Color::red()};
    }

    void DrawIcon(EntryStyling const& style)
    {
        PushStyleColor(ImGuiCol_Text, style.color);
        TextUnformatted(style.icon);
        PopStyleColor();
    }

    void DrawEntryIconAndText(
        UIState const&,
        OpenSim::Component const& component,
        EntryStyling style)
    {
        DrawIcon(style);
        ImGui::SameLine();
        TextUnformatted(component.getName());
    }

    template<DerivedFrom<OpenSim::Component> T>
    void DrawEntryIconAndText(UIState const& state, T const& component)
    {
        DrawEntryIconAndText(state, component, CalcStyle(state, component));
    }

    void DrawTooltipHeader(UIState const&, OpenSim::Component const& component)
    {
        TextUnformatted(GetAbsolutePathString(component));
        ImGui::SameLine();
        ImGui::TextDisabled("%s", component.getConcreteClassName().c_str());
        ImGui::Separator();
        ImGui::Dummy({0.0f, 3.0f});
    }
}

// UI (meshes/mesh pairing)
namespace
{
    void DrawDetailsTable(
        OpenSim::Mesh const& mesh,
        MeshWarpPairing const* maybePairing)
    {
        if (ImGui::BeginTable("##Details", 2))
        {
            ImGui::TableSetupColumn("Label");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ForEachDetailIn(mesh, maybePairing, [](PairingDetail detail)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                TextUnformatted(detail.name);
                ImGui::TableSetColumnIndex(1);
                TextUnformatted(detail.value);
            });
            ImGui::EndTable();
        }
    }

    void DrawMeshTooltipChecklist(MeshWarpPairing const* maybePairing)
    {
        ImGui::Indent(5.0f);
        int id = 0;
        ForEachCheck(maybePairing, [&id](InputCheck check)
        {
            ImGui::PushID(id);
            auto style = ToStyle(check.state);
            DrawIcon(style);
            ImGui::SameLine();
            TextUnformatted(check.description);
            ImGui::PopID();
            return SearchState::Continue;
        });
        ImGui::Unindent(5.0f);
    }

    void DrawTooltipContent(UIState const& state, OpenSim::Mesh const& mesh)
    {
        DrawTooltipHeader(state, mesh);

        MeshWarpPairing const* maybePairing = state.findMeshWarp(mesh);

        ImGui::Text("Checklist:");
        ImGui::Dummy({0.0f, 3.0f});
        DrawMeshTooltipChecklist(maybePairing);

        ImGui::NewLine();

        ImGui::Text("Details:");
        ImGui::Dummy({0.0f, 3.0f});
        DrawDetailsTable(mesh, maybePairing);
    }

    void DrawMeshEntry(UIState const& state, OpenSim::Mesh const& mesh)
    {
        DrawEntryIconAndText(state, mesh);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        {
            ImGui::BeginTooltip();
            DrawTooltipContent(state, mesh);
            ImGui::EndTooltip();
        }
    }

    void DrawMeshSection(UIState const& state)
    {
        auto ptrs = state.getWarpableMeshes();

        ImGui::Text("Meshes");
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu)", ptrs.size());
        ImGui::SameLine();
        DrawHelpMarker("Shows which meshes are present in the source model, whether they have associated landmarks, and whether there is a known destination mesh");

        ImGui::Separator();

        int id = 0;
        for (OpenSim::Mesh const* mesh : ptrs)
        {
            ImGui::PushID(id++);
            DrawMeshEntry(state, *mesh);
            ImGui::PopID();
        }
    }
}

// UI (frames)
namespace
{
    void DrawTooltipContent(UIState const& state, OpenSim::Frame const& frame)
    {
        DrawTooltipHeader(state, frame);
    }

    void DrawFrameEntry(UIState const& state, OpenSim::Frame const& frame)
    {
        DrawEntryIconAndText(state, frame);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        {
            BeginTooltip();
            DrawTooltipContent(state, frame);
            EndTooltip();
        }
    }

    void DrawFramesSection(UIState const& state)
    {
        auto ptrs = state.getWarpableFrames();

        ImGui::Text("Frames");
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu)", ptrs.size());
        ImGui::SameLine();
        DrawHelpMarker("Shows frames in the source model, and whether they are warp-able or not");

        ImGui::Separator();

        int id = 0;
        for (OpenSim::Frame const* frame : ptrs)
        {
            ImGui::PushID(id++);
            DrawFrameEntry(state, *frame);
            ImGui::PopID();
        }
    }
}


// public API

void osc::mow::ChecklistPanel::implDrawContent()
{
    int id = 0;

    ImGui::PushID(id++);
    DrawMeshSection(*m_State);
    ImGui::PopID();

    ImGui::NewLine();

    ImGui::PushID(id++);
    DrawFramesSection(*m_State);
    ImGui::PopID();
}
