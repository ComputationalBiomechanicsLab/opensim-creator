#include "component_hierarchy.hpp"

#include "src/assertions.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

static size_t framegeom_hash = typeid(OpenSim::FrameGeometry).hash_code();
static size_t wos_hash = typeid(OpenSim::WrapObjectSet).hash_code();
static constexpr size_t max_depth = 16;

// poor-man's abstraction for a constant-sized array
template<typename T, size_t N>
struct Sized_array final {
    static_assert(std::is_trivially_destructible_v<T>);

    std::array<T, N> els;
    size_t n = 0;

    Sized_array& operator=(Sized_array const& rhs) {
        std::copy(rhs.begin(), rhs.end(), els.begin());
        n = rhs.size();
        return *this;
    }

    void push_back(T v) {
        if (n >= N) {
            throw std::runtime_error{"cannot render a component_hierarchy widget: the Model/Component tree is too deep"};
        }
        els[n++] = v;
    }

    auto begin() const -> decltype(els.begin()) {
        return els.begin();
    }

    auto begin() -> decltype(els.begin()) {
        return els.begin();
    }

    auto end() const -> decltype(els.begin() + n) {
        return els.begin() + n;
    }

    auto end() -> decltype(els.begin() + n) {
        return els.begin() + n;
    }

    size_t size() const {
        return n;
    }

    bool empty() const {
        return n == 0;
    }

    void resize(size_t newsize) {
        n = newsize;
    }

    void clear() {
        n = 0;
    }

    T& operator[](size_t n) {
        return els[n];
    }

    T const& operator[](size_t i) const {
        return els[i];
    }
};

// appends components to the outparam with a sequence of Open
// that lead from `root` to `cr`
static void component_path_ptrs(OpenSim::Component const* root,
                                OpenSim::Component const* cr,
                                Sized_array<OpenSim::Component const*, max_depth>& out) {
    if (!cr) {
        return;
    }

    // child --> parent
    OpenSim::Component const* c = cr;
    while (c != root && c->hasOwner()) {
        out.push_back(c);
        c = &c->getOwner();
    }

    // parent --> child
    std::reverse(out.begin(), out.end());
}

osc::ui::component_hierarchy::Response osc::ui::component_hierarchy::draw(
    OpenSim::Component const* root,
    OpenSim::Component const* current_selection,
    OpenSim::Component const* current_hover) {

    // precompute selection + hover paths
    //
    // this is so that we can query whether something in the hierarchy
    // intersects the selection/hover path (for highlighting, auto-opening,
    // etc.)
    Sized_array<OpenSim::Component const*, max_depth> selection_path;
    component_path_ptrs(root, current_selection, selection_path);
    Sized_array<OpenSim::Component const*, max_depth> hover_path;
    component_path_ptrs(root, current_hover, hover_path);

    Sized_array<OpenSim::Component const*, max_depth> prev;
    OpenSim::Component const* prev_component = nullptr;
    Sized_array<OpenSim::Component const*, max_depth> cur;
    Sized_array<bool, max_depth> enabled_nodes;
    int imgui_id = 0;
    Response response;

    // this implementation is strongly dependent on OpenSim's component
    // list traversal algorithm, which is a depth-first traversal that
    // stops at each parent *before* traversing

    for (OpenSim::Component const& cr : root->getComponentList()) {
        // apply filters
        {
            auto cr_hash = typeid(cr).hash_code();
            if (cr_hash == framegeom_hash || cr_hash == wos_hash) {
                continue;
            }
        }

        // compute path
        component_path_ptrs(root, &cr, cur);

        // everything we are doing next assumes there is a *previous* path
        if (prev.empty()) {
            prev = cur;
            prev_component = &cr;
            cur.clear();
            continue;
        }

        OSC_ASSERT_ALWAYS(enabled_nodes.size() == prev.size()-1);

        // figure out if `prev` should actually shown in the UI
        //
        // this is because the user can collapse parent headers etc.
        bool prev_should_show =
            !std::any_of(enabled_nodes.begin(), enabled_nodes.end(), [](bool b) { return !b; });

        if (prev_should_show) {
            // figure out if a component in the current selection/hover is
            // the `prev` component being rendered, so that automatic tree
            // opening etc. works


            ImGui::PushID(imgui_id++);

            bool prev_in_selection_pth =
                std::find(selection_path.begin(), selection_path.end(), prev_component) != selection_path.end();

            if (prev.size() < cur.size() || prev.size() == 1) {
                if (prev_in_selection_pth) {
                    ImGui::SetNextItemOpen(true);
                }
                int styles = 0;
                if (prev_component == current_selection) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
                    ++styles;
                }

                bool shown = ImGui::TreeNode(prev_component->getName().c_str());
                enabled_nodes.push_back(shown);

                ImGui::PopStyleColor(styles);
            } else {

                bool prev_in_hover_pth =
                    std::find(hover_path.begin(), hover_path.end(), prev_component) != hover_path.end();

                int styles = 0;
                if (prev_in_hover_pth) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.5f, 0.5f, 0.0f, 1.0f});
                    ++styles;
                }
                if (prev_in_selection_pth) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
                    ++styles;
                }

                ImGui::TextUnformatted(prev_component->getName().c_str());

                ImGui::PopStyleColor(styles);
            }

            if (ImGui::IsItemHovered()) {
                response.type = HoverChanged;
                response.ptr = prev_component;
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                response.type = SelectionChanged;
                response.ptr = prev_component;
            }

            ImGui::PopID();
        } else {
            if (prev.size() < cur.size()) {
                enabled_nodes.push_back(false);
            }
        }

        if ((prev.size() > cur.size() || prev.size() == 1) && cur.size() > 0) {
            for (auto it = enabled_nodes.begin() + (cur.size()-1); it != enabled_nodes.end(); ++it) {
                if (*it) {
                    ImGui::TreePop();
                }
            }
            enabled_nodes.resize(cur.size()-1);
        }

        // update loop invariants
        prev = cur;
        prev_component = &cr;
        cur.clear();
    }

    for (bool b : enabled_nodes) {
        if (b) {
            ImGui::TreePop();
        }
    }

    return response;
}
