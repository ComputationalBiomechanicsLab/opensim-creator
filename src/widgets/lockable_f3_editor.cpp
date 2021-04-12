#include "lockable_f3_editor.hpp"

#include <SimTKcommon.h>
#include <imgui.h>

template<typename Coll1, typename Coll2>
static float diff(Coll1 const& older, Coll2 const& newer, size_t n) {
    for (int i = 0; i < static_cast<int>(n); ++i) {
        if (static_cast<float>(older[i]) != static_cast<float>(newer[i])) {
            return newer[i];
        }
    }
    return static_cast<float>(older[0]);
}

bool osc::draw_lockable_f3_editor(char const* lock_id, char const* editor_id, float* v, bool* is_locked) {
    bool changed = false;

    if (ImGui::Checkbox(lock_id, is_locked)) {
        changed = true;
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    float copy[3];
    copy[0] = v[0];
    copy[1] = v[1];
    copy[2] = v[2];

    if (ImGui::InputFloat3(editor_id, copy, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (*is_locked) {
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
