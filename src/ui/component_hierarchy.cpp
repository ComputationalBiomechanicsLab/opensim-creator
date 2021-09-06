#include "component_hierarchy.hpp"

#include "src/Assertions.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

namespace {
    size_t const g_FrameGeometryHash = typeid(OpenSim::FrameGeometry).hash_code();
    size_t const g_WrapObjectSetHash = typeid(OpenSim::WrapObjectSet).hash_code();

    // poor-man's abstraction for a constant-sized array
    template<typename T, size_t N>
    struct Sized_array final {
        static_assert(std::is_trivial_v<T>);
        static_assert(N <= std::numeric_limits<int>::max());

        std::array<T, N> els;
        int n = 0;

        Sized_array& operator=(Sized_array const& rhs) {
            std::copy(rhs.els.begin(), rhs.els.begin() + size(), els.begin());
            n = rhs.n;
            return *this;
        }

        void push_back(T v) {
            if (static_cast<size_t>(n) >= N) {
                throw std::runtime_error{"cannot render a component_hierarchy widget: the Model/Component tree is too deep"};
            }
            els[n++] = v;
        }

        T const* begin() const noexcept {
            return els.data();
        }

        T* begin() noexcept {
            return els.data();
        }

        T const* end() const noexcept {
            return els.data() + static_cast<size_t>(n);
        }

        T* end() noexcept {
            return els.data() + static_cast<size_t>(n);
        }

        size_t size() const noexcept {
            return static_cast<size_t>(n);
        }

        int sizei() const noexcept {
            return n;
        }

        bool empty() const noexcept {
            return n == 0;
        }

        void resize(size_t newsize) noexcept {
            n = newsize;
        }

        void clear() noexcept {
            n = 0;
        }

        T& operator[](size_t idx) {
            return els[idx];
        }

        T const& operator[](size_t i) const noexcept {
            return els[i];
        }
    };

    using Component_path = Sized_array<OpenSim::Component const*, 16>;

    // populates `out` with the sequence of nodes between (ancestor..child]
    void compute_path(
            OpenSim::Component const* ancestor,
            OpenSim::Component const* child,
            Component_path& out) {

        out.clear();

        // child --> parent
        while (child != ancestor && child->hasOwner()) {
            out.push_back(child);
            child = &child->getOwner();
        }

        // parent --> child
        std::reverse(out.begin(), out.end());
    }

    bool path_contains(Component_path const& p, OpenSim::Component const* c) {
        return std::find(p.begin(), p.end(), c) != p.end();
    }

    bool should_render(OpenSim::Component const& c) {
        auto hc = typeid(c).hash_code();
        return hc != g_FrameGeometryHash && hc != g_WrapObjectSetHash;
    }
}


// public API

osc::ui::component_hierarchy::Response osc::ui::component_hierarchy::draw(
    OpenSim::Component const* root,
    OpenSim::Component const* selection,
    OpenSim::Component const* hover) {

    Response response;

    if (!root) {
        return response;
    }

    Component_path selection_path;
    if (selection) {
        compute_path(root, selection, selection_path);
    }

    Component_path hover_path;
    if (hover) {
        compute_path(root, hover, hover_path);
    }

    // init iterators: this alg. is single-pass with a 1-token lookahead
    auto const lst = root->getComponentList();
    auto it = lst.begin();
    auto const end = lst.end();

    // initially populate lookahead (+ path)
    OpenSim::Component const* lookahead = nullptr;
    Component_path lookahead_path;
    while (it != end) {
        OpenSim::Component const& c = *it++;

        if (should_render(c)) {
            lookahead = &c;
            compute_path(root, &c, lookahead_path);
            break;
        }
    }

    OpenSim::Component const* cur = nullptr;
    Component_path cur_path;

    int imgui_tree_depth = 0;
    int imgui_id = 0;

    while (lookahead) {
        // important: ensure all nodes have a unique ID: regardess of filtering
        ++imgui_id;

        // populate current (+ path) from lookahead
        cur = lookahead;
        cur_path = lookahead_path;

        OSC_ASSERT(cur && "cur ptr should *definitely* be populated at this point");
        OSC_ASSERT(!cur_path.empty() && "current path cannot be empty (even a root element has a path)");

        // update lookahead (+ path)
        lookahead = nullptr;
        lookahead_path.clear();
        while (it != end) {
            OpenSim::Component const& c = *it++;

            if (should_render(c)) {
                lookahead = &c;
                compute_path(root, &c, lookahead_path);
                break;
            }
        }
        OSC_ASSERT((lookahead || !lookahead) && "a lookahead is not *required* at this point");

        // skip rendering if a parent node is collapsed
        if (imgui_tree_depth < cur_path.sizei() - 1) {
            continue;
        }

        // pop tree nodes down to the current depth
        while (imgui_tree_depth >= cur_path.sizei()) {
            ImGui::TreePop();
            --imgui_tree_depth;
        }
        OSC_ASSERT(imgui_tree_depth == cur_path.sizei() - 1);


        if (cur_path.size() < 2 || lookahead_path.size() > cur_path.size()) {
            // render as an expandable tree node

            int styles = 0;
            if (cur == hover) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.5f, 0.5f, 0.0f, 1.0f});
                ++styles;
            }
            if (cur == selection) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
                ++styles;
            }

            if (path_contains(selection_path, cur)) {
                ImGui::SetNextItemOpen(true);
            }

            ImGui::PushID(imgui_id);
            if (ImGui::TreeNode(cur->getName().c_str())) {
                ++imgui_tree_depth;
            }
            ImGui::PopID();
            ImGui::PopStyleColor(styles);
        } else {
            // render as plain text

            int styles = 0;
            if (path_contains(hover_path, cur)) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.5f, 0.5f, 0.0f, 1.0f});
                ++styles;
            }
            if (path_contains(selection_path, cur)) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
                ++styles;
            }

            ImGui::PushID(imgui_id);
            ImGui::TextUnformatted(cur->getName().c_str());
            ImGui::PopID();
            ImGui::PopStyleColor(styles);
        }

        if (ImGui::IsItemHovered()) {
            response.type = HoverChanged;
            response.ptr = cur;
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            response.type = SelectionChanged;
            response.ptr = cur;
        }
    }

    // pop remaining dangling tree elements
    while (imgui_tree_depth-- > 0) {
        ImGui::TreePop();
    }

    return response;
}
