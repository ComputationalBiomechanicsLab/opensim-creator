#include "hierarchy_viewer.hpp"

#include <imgui.h>
#include <OpenSim/Common/Component.h>

#include <string>
#include <array>

void osmv::Hierarchy_viewer::draw(const OpenSim::Component *root, OpenSim::Component const** selected, OpenSim::Component const** hovered) {
    std::string output_str;
    std::array<OpenSim::Component const*, 32> prev_path;
    size_t prev_path_sz = 0;
    bool header_showing = true;
    for (OpenSim::Component const& cr : root->getComponentList()) {

        std::array<OpenSim::Component const*, 32> cur_path;
        size_t i = 0;

        {
            // push each element in the tree into a stack (child --> parent)
            OpenSim::Component const* c = &cr;
            while (c != root) {
                assert(i < cur_path.size());
                cur_path[i++] = c;
                c = &c->getOwner();
            }

            // reverse the stack to yield a linear sequence (parent --> child)
            std::reverse(cur_path.begin(), cur_path.begin() + i);
        }

        size_t indent = 0;
        {
            // figure out common subpath between this path and the previous one
            size_t len = std::min(prev_path_sz, i);
            while (indent < len and prev_path[indent] == cur_path[indent]) {
                ++indent;
            }
        }

        if (indent == 0) {
            // edge-case: first-level elements should be put into a collapsing header that
            //            the user can toggle
            header_showing = ImGui::CollapsingHeader(cur_path[indent]->getName().c_str());
            ++indent;
        }

        if (header_showing) {
            for (size_t j = indent; j < i; ++j) {
                // print out the not-common tail

                OpenSim::Component const* comp = cur_path[j];

                output_str.clear();
                for (size_t k = 0; k < indent; ++k) {
                    output_str += "    ";
                }
                output_str += comp->getName().c_str();

                int style_pushes = 0;
                if (comp == *hovered) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.5f, 0.5f, 0.0f, 1.0f});
                    ++style_pushes;
                }
                if (comp == *selected) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
                    ++style_pushes;
                }

                ImGui::Text(output_str.c_str());
                if (ImGui::IsItemHovered()) {
                    *hovered = comp;
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    *selected = comp;
                }

                ImGui::PopStyleColor(style_pushes);
            }
        }

        {
            // update loop invariants
            std::copy(cur_path.begin(), cur_path.begin() + i, prev_path.begin());
            prev_path_sz = i;
        }
    }
}
