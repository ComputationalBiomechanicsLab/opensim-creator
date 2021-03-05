#include "component_hierarchy_widget.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <string>

void osmv::Component_hierarchy_widget::draw(
    const OpenSim::Component* root, OpenSim::Component const** selected, OpenSim::Component const** hovered) {

    OpenSim::Component const* selection_top_level_parent = nullptr;
    if (selected != nullptr and (*selected) != nullptr) {
        OpenSim::Component const* c = *selected;
        while (&c->getOwner() != root) {
            c = &c->getOwner();
        }
        selection_top_level_parent = c;
    }

    std::string swap;
    std::array<OpenSim::Component const*, 32> prev_path_els;
    size_t prev_num_path_els = 0;
    int id = 0;
    bool header_showing = false;

    for (OpenSim::Component const& cr : root->getComponentList()) {
        if (dynamic_cast<OpenSim::FrameGeometry const*>(&cr)) {
            continue;  // HACK: don't list FrameGeometry
        }

        // break the path up into individual components
        std::array<OpenSim::Component const*, 32> path_els;
        static_assert(path_els.size() == prev_path_els.size());
        size_t num_path_els = 0;
        {
            // push each element in the tree into a stack (child --> parent)
            OpenSim::Component const* c = &cr;
            while (c != root) {
                assert(num_path_els < path_els.size());
                path_els[num_path_els++] = c;
                c = &c->getOwner();
            }

            // reverse the stack to yield a linear sequence (parent --> child)
            std::reverse(path_els.begin(), path_els.begin() + num_path_els);
        }

        // figure out the disjoint location between this path and the previous one
        size_t disjoint_begin = 0;
        {
            size_t len = std::min(prev_num_path_els, num_path_els);
            while (disjoint_begin < len and prev_path_els[disjoint_begin] == path_els[disjoint_begin]) {
                ++disjoint_begin;
            }
        }

        if (bool is_root_header = disjoint_begin == 0; is_root_header) {
            if (header_showing) {
                ImGui::TreePop();  // from last push
            }

            OpenSim::Component const* comp = path_els[0];

            int style_pushes = 0;
            if (comp == *selected) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
                ++style_pushes;
            }

            if (comp == selection_top_level_parent) {
                ImGui::SetNextItemOpen(true);
            }
            header_showing = ImGui::TreeNode(path_els[0]->getName().c_str());
            ++disjoint_begin;

            if (ImGui::IsItemHovered()) {
                *hovered = comp;
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                *selected = comp;
            }

            ImGui::PopStyleColor(style_pushes);
        }

        if (header_showing) {
            for (size_t i = disjoint_begin; i < num_path_els; ++i) {
                OpenSim::Component const* comp = path_els[i];

                swap.clear();
                for (size_t k = 0; k < disjoint_begin; ++k) {
                    swap += "  ";
                }
                swap += comp->getName().c_str();

                int style_pushes = 0;
                if (comp == *hovered) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.5f, 0.5f, 0.0f, 1.0f});
                    ++style_pushes;
                }
                if (comp == *selected) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
                    ++style_pushes;
                }

                ImGui::PushID(id++);
                ImGui::Text("%s", swap.c_str());
                ImGui::PopID();
                if (ImGui::IsItemHovered()) {
                    *hovered = comp;
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    *selected = comp;
                }

                ImGui::PopStyleColor(style_pushes);
            }
        }

        // update loop invariants
        {
            std::copy(path_els.begin(), path_els.begin() + num_path_els, prev_path_els.begin());
            prev_num_path_els = num_path_els;
        }
    }

    if (header_showing) {
        ImGui::TreePop();
    }
}
