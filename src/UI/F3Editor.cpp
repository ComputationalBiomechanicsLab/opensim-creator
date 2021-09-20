#include "F3Editor.hpp"

#include "src/Styling.hpp"

#include <SimTKcommon.h>
#include <imgui.h>

namespace {
    template<typename Coll1, typename Coll2>
    float diff(Coll1 const& older, Coll2 const& newer, size_t n) {
        for (int i = 0; i < static_cast<int>(n); ++i) {
            if (static_cast<float>(older[i]) != static_cast<float>(newer[i])) {
                return newer[i];
            }
        }
        return static_cast<float>(older[0]);
    }
}

// public API

bool osc::DrawF3Editor(char const* lockID, char const* editorID, float* v, bool* isLocked) {
    bool changed = false;

    ImGui::PushID(*lockID);
    if (ImGui::Button(*isLocked ? ICON_FA_LOCK : ICON_FA_UNLOCK)) {
        *isLocked = !*isLocked;
        changed = true;
    }
    ImGui::PopID();

    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    float copy[3];
    copy[0] = v[0];
    copy[1] = v[1];
    copy[2] = v[2];

    if (ImGui::InputFloat3(editorID, copy, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (*isLocked) {
            float val = diff(v, copy, 3);
            v[0] = val;
            v[1] = val;
            v[2] = val;
        } else {
            v[0] = copy[0];
            v[1] = copy[1];
            v[2] = copy[2];
        }
        changed = true;
    }

    return changed;
}
