#include "ComponentHierarchy.hpp"

#include "src/Utils/Algorithms.hpp"
#include "src/Assertions.hpp"
#include "src/Log.hpp"
#include "src/Styling.hpp"

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
    struct SizedArray final {
        static_assert(std::is_trivial_v<T>);
        static_assert(N <= std::numeric_limits<int>::max());

        std::array<T, N> els;
        int n = 0;

        SizedArray& operator=(SizedArray const& rhs) {
            std::copy(rhs.els.begin(), rhs.els.begin() + rhs.size(), els.begin());
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

    using ComponentPath = SizedArray<OpenSim::Component const*, 16>;

    // populates `out` with the sequence of nodes between (ancestor..child]
    void computeComponentPath(
            OpenSim::Component const* ancestor,
            OpenSim::Component const* child,
            ComponentPath& out) {

        out.clear();

        // populate child --> parent
        while (child) {
            out.push_back(child);

            if (!child->hasOwner()) {
                break;
            }

            if (child == ancestor) {
                break;
            }

            child = &child->getOwner();
        }

        // reverse to yield parent --> child
        std::reverse(out.begin(), out.end());
    }

    bool pathContains(ComponentPath const& p, OpenSim::Component const* c) {
        return std::find(p.begin(), p.end(), c) != p.end();
    }
}

static bool isSearchHit(std::string searchStr, ComponentPath const& cp) {
    for (auto const& c : cp) {
        if (osc::ContainsSubstringCaseInsensitive(c->getName(), searchStr)) {
            return true;
        }
    }
    return false;
}


// public API

osc::ComponentHierarchy::Response osc::ComponentHierarchy::draw(
    OpenSim::Component const* root,
    OpenSim::Component const* selection,
    OpenSim::Component const* hover) {

    ImGui::Dummy({0.0f, 3.0f});
    ImGui::TextUnformatted(ICON_FA_EYE);
    if (ImGui::BeginPopupContextItem("##filterpopup")) {
        ImGui::Checkbox("frames", &showFrames);
        ImGui::Checkbox("wrapobjectsets", &showWrapObjectSets);
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (search[0] != '\0') {
        if (ImGui::Button("X")) {
            search[0] = '\0';
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Clear the search string");
            ImGui::EndTooltip();
        }
    } else {
        ImGui::TextUnformatted(ICON_FA_SEARCH);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    ImGui::InputText("##hirarchtsearchbar", search, sizeof(search));
    ImGui::Dummy({0.0f, 3.0f});
    ImGui::Separator();
    ImGui::Dummy({0.0f, 3.0f});

    ImGui::BeginChild("##componenthierarchyvieweritems");

    Response response;

    if (!root) {
        return response;
    }

    ComponentPath selectionPath;
    if (selection) {
        computeComponentPath(root, selection, selectionPath);
    }

    ComponentPath hoverPath;
    if (hover) {
        computeComponentPath(root, hover, hoverPath);
    }

    // init iterators: this alg. is single-pass with a 1-token lookahead
    auto const lst = root->getComponentList();
    auto it = lst.begin();
    auto const end = lst.end();

    // initially populate lookahead (+ path)
    OpenSim::Component const* lookahead = root;
    ComponentPath lookaheadPath;
    computeComponentPath(root, root, lookaheadPath);

    // set cur path empty (first step copies lookahead into this)
    OpenSim::Component const* cur = nullptr;
    ComponentPath currentPath;

    int imguiTreeDepth = 0;
    int imguiId = 0;
    bool hasSearch = std::strlen(search) > 0;

    float unindentPerLevel = ImGui::GetTreeNodeToLabelSpacing() - 15.0f;

    while (lookahead) {
        // important: ensure all nodes have a unique ID: regardess of filtering
        ++imguiId;

        // populate current (+ path) from lookahead
        cur = lookahead;
        currentPath = lookaheadPath;

        OSC_ASSERT(cur && "cur ptr should *definitely* be populated at this point");
        OSC_ASSERT(!currentPath.empty() && "current path cannot be empty (even a root element has a path)");

        // update lookahead (+ path) by stepping to the next component in the component tree
        lookahead = nullptr;
        lookaheadPath.clear();
        while (it != end) {
            OpenSim::Component const& c = *it++;

            bool shouldRender = true;

            auto hc = typeid(c).hash_code();
            if (!showFrames && hc == g_FrameGeometryHash) {
                shouldRender = false;
            }

            if (!showWrapObjectSets && hc == g_WrapObjectSetHash) {
                shouldRender = false;
            }

            if (shouldRender) {
                lookahead = &c;
                computeComponentPath(root, &c, lookaheadPath);
                break;
            }
        }
        OSC_ASSERT((lookahead || !lookahead) && "a lookahead is not *required* at this point");

        bool searchHit = hasSearch && isSearchHit(search, currentPath);

        // skip rendering if a parent node is collapsed
        if (imguiTreeDepth < currentPath.sizei() - 1) {
            continue;
        }

        // pop tree nodes down to the current depth
        while (imguiTreeDepth >= currentPath.sizei()) {
            ImGui::Indent(unindentPerLevel);
            ImGui::TreePop();
            --imguiTreeDepth;
        }
        OSC_ASSERT(imguiTreeDepth <= currentPath.sizei() - 1);

        // handle display mode (node vs leaf)
        bool isInternalNode = currentPath.size() < 3 || lookaheadPath.size() > currentPath.size();
        ImGuiTreeNodeFlags nodeFlags = isInternalNode ? 0 : ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;

        // handle coloring
        int styles = 0;
        if (cur == selection) {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_SELECTED_COMPONENT_RGBA);
            ++styles;
        } else if (cur == hover) {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_HOVERED_COMPONENT_RGBA);
            ++styles;
        } else if (!hasSearch || searchHit) {
            // display as normal
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_GREYED_RGBA);
            ++styles;
        }

        // auto-open in these cases
        if (searchHit || currentPath.sizei() == 1 || pathContains(selectionPath, cur)) {
            ImGui::SetNextItemOpen(true);
        }

        ImGui::PushID(imguiId);
        if (ImGui::TreeNodeEx(cur->getName().c_str(), nodeFlags)) {
            ImGui::Unindent(unindentPerLevel);
            ++imguiTreeDepth;
        }
        ImGui::PopID();
        ImGui::PopStyleColor(styles);

        if (ImGui::IsItemHovered()) {
            response.type = HoverChanged;
            response.ptr = cur;

            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);
            ImGui::TextUnformatted(cur->getConcreteClassName().c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            response.type = SelectionChanged;
            response.ptr = cur;
        }
    }

    // pop remaining dangling tree elements
    while (imguiTreeDepth-- > 0) {
        ImGui::Indent(unindentPerLevel);
        ImGui::TreePop();
    }

    ImGui::EndChild();

    return response;
}
