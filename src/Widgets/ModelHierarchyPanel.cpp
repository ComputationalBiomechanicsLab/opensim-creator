#include "ModelHierarchyPanel.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Widgets/NamedPanel.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>
#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <utility>

namespace {
    size_t const g_FrameGeometryHash = typeid(OpenSim::FrameGeometry).hash_code();

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
        auto end = p.begin() == p.end() ? p.end() : p.end()-1;
        return std::find(p.begin(), end, c) != end;
    }
}

static bool isSearchHit(std::string const& searchStr, ComponentPath const& cp)
{
    for (auto const& c : cp)
    {
        if (osc::ContainsSubstringCaseInsensitive(c->getName(), searchStr))
        {
            return true;
        }
    }
    return false;
}

class osc::ModelHierarchyPanel::Impl final : public NamedPanel {
public:
    Impl(std::string_view panelName) :NamedPanel{std::move(panelName)}
    {
    }

    bool isOpen() const
    {
        return static_cast<NamedPanel const&>(*this).isOpen();
    }

    void open()
    {
        return static_cast<NamedPanel&>(*this).open();
    }

    void close()
    {
        return static_cast<NamedPanel&>(*this).close();
    }

    osc::ModelHierarchyPanel::Response draw(VirtualConstModelStatePair const& modelState)
    {
        m_Response = Response{};
        m_ModelState = &modelState;
        static_cast<NamedPanel&>(*this).draw();
        m_ModelState = nullptr;
        return m_Response;
    }

    void implDraw() override
    {
        ImGui::Dummy({0.0f, 3.0f});

        // draw filter stuff

        ImGui::TextUnformatted(ICON_FA_EYE);
        if (ImGui::BeginPopupContextItem("##filterpopup"))
        {
            ImGui::Checkbox("frames", &m_ShowFrames);
            ImGui::EndPopup();
        }
        ImGui::SameLine();

        if (!m_CurrentSearch.empty())
        {
            if (ImGui::Button("X"))
            {
                m_CurrentSearch.clear();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Clear the search string");
                ImGui::EndTooltip();
            }
        }
        else
        {
            ImGui::TextUnformatted(ICON_FA_SEARCH);
        }

        // draw search bar

        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
        osc::InputString("##hirarchtsearchbar", m_CurrentSearch, 256);
        ImGui::Dummy({0.0f, 3.0f});
        ImGui::Separator();
        ImGui::Dummy({0.0f, 3.0f});

        // draw content

        ImGui::BeginChild("##componenthierarchyvieweritems");

        OpenSim::Component const* root = &m_ModelState->getModel();
        OpenSim::Component const* selection = m_ModelState->getSelected();
        OpenSim::Component const* hover = m_ModelState->getHovered();

        ComponentPath selectionPath;
        if (selection)
        {
            computeComponentPath(root, selection, selectionPath);
        }

        ComponentPath hoverPath;
        if (hover)
        {
            computeComponentPath(root, hover, hoverPath);
        }

        Response response;

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
        bool hasSearch = !m_CurrentSearch.empty();

        float unindentPerLevel = ImGui::GetTreeNodeToLabelSpacing() - 15.0f;

        while (lookahead)
        {
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
            while (it != end)
            {
                OpenSim::Component const& c = *it++;

                bool shouldRender = true;

                auto hc = typeid(c).hash_code();
                if (!m_ShowFrames && hc == g_FrameGeometryHash)
                {
                    shouldRender = false;
                }
                else if (auto const* wos = dynamic_cast<OpenSim::WrapObjectSet const*>(&c))
                {
                    shouldRender = wos->getSize() > 0;
                }
                else if (!ShouldShowInUI(c))
                {
                    shouldRender = false;
                }

                if (shouldRender)
                {
                    lookahead = &c;
                    computeComponentPath(root, &c, lookaheadPath);
                    break;
                }
            }
            OSC_ASSERT((lookahead || !lookahead) && "a lookahead is not *required* at this point");

            bool searchHit = hasSearch && isSearchHit(m_CurrentSearch, currentPath);

            // skip rendering if a parent node is collapsed
            if (imguiTreeDepth < currentPath.sizei() - 1)
            {
                continue;
            }

            // pop tree nodes down to the current depth
            while (imguiTreeDepth >= currentPath.sizei())
            {
                ImGui::Indent(unindentPerLevel);
                ImGui::TreePop();
                --imguiTreeDepth;
            }
            OSC_ASSERT(imguiTreeDepth <= currentPath.sizei() - 1);

            // handle display mode (node vs leaf)
            bool isInternalNode = currentPath.size() < 3 || lookaheadPath.size() > currentPath.size();
            ImGuiTreeNodeFlags nodeFlags = isInternalNode ? ImGuiTreeNodeFlags_OpenOnArrow : (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet);

            // handle coloring
            int styles = 0;
            if (cur == selection)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OSC_SELECTED_COMPONENT_RGBA);
                ++styles;
            }
            else if (cur == hover)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OSC_HOVERED_COMPONENT_RGBA);
                ++styles;
            }
            else if (!hasSearch || searchHit)
            {
                // display as normal
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OSC_GREYED_RGBA);
                ++styles;
            }

            // auto-open in these cases
            if (searchHit || currentPath.sizei() == 1 || pathContains(selectionPath, cur))
            {
                ImGui::SetNextItemOpen(true);
            }

            ImGui::PushID(imguiId);
            if (ImGui::TreeNodeEx(cur->getName().c_str(), nodeFlags))
            {
                ImGui::Unindent(unindentPerLevel);
                ++imguiTreeDepth;
            }
            ImGui::PopID();
            ImGui::PopStyleColor(styles);

            if (ImGui::IsItemHovered())
            {
                m_Response.type = ModelHierarchyPanel::ResponseType::HoverChanged;
                m_Response.ptr = cur;

                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);
                ImGui::TextUnformatted(cur->getConcreteClassName().c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                m_Response.type = ModelHierarchyPanel::ResponseType::SelectionChanged;
                m_Response.ptr = cur;
            }
        }

        // pop remaining dangling tree elements
        while (imguiTreeDepth-- > 0)
        {
            ImGui::Indent(unindentPerLevel);
            ImGui::TreePop();
        }

        ImGui::EndChild();
    }

private:
    std::string m_CurrentSearch;
    bool m_ShowFrames = false;
    VirtualConstModelStatePair const* m_ModelState = nullptr;
    ModelHierarchyPanel::Response m_Response;
};


// public API (PIMPL)

osc::ModelHierarchyPanel::ModelHierarchyPanel(std::string_view panelName) :
    m_Impl{new Impl{std::move(panelName)}}
{
}

osc::ModelHierarchyPanel::ModelHierarchyPanel(ModelHierarchyPanel&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ModelHierarchyPanel& osc::ModelHierarchyPanel::operator=(ModelHierarchyPanel&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ModelHierarchyPanel::~ModelHierarchyPanel() noexcept
{
    delete m_Impl;
}

bool osc::ModelHierarchyPanel::isOpen() const
{
    return m_Impl->isOpen();
}

void osc::ModelHierarchyPanel::open()
{
    m_Impl->open();
}

void osc::ModelHierarchyPanel::close()
{
    m_Impl->close();
}

osc::ModelHierarchyPanel::Response osc::ModelHierarchyPanel::draw(VirtualConstModelStatePair const& modelState)
{
    return m_Impl->draw(modelState);
}
