#include "NavigatorPanel.hpp"

#include "OpenSimCreator/Model/VirtualModelStatePair.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "OpenSimCreator/Widgets/BasicWidgets.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Panels/StandardPanel.hpp>
#include <oscar/Platform/Styling.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <imgui.h>
#include <IconsFontAwesome5.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <utility>

namespace
{
    // poor-man's abstraction for a constant-sized array
    template<typename T, size_t N>
    class SizedArray final {
        static_assert(std::is_trivial_v<T>);
        static_assert(N <= std::numeric_limits<int>::max());

    public:
        void push_back(T v) {
            if (m_Size >= N) {
                throw std::runtime_error{"cannot render a navigator: the Model/Component tree is too deep"};
            }
            m_Data[m_Size++] = v;
        }

        T const* begin() const noexcept {
            return m_Data.data();
        }

        T* begin() noexcept {
            return m_Data.data();
        }

        T const* end() const noexcept {
            return m_Data.data() + m_Size;
        }

        T* end() noexcept {
            return m_Data.data() + m_Size;
        }

        size_t size() const noexcept {
            return m_Size;
        }

        ptrdiff_t sizei() const noexcept {
            return static_cast<ptrdiff_t>(m_Size);
        }

        bool empty() const noexcept {
            return m_Size == 0;
        }

        void resize(size_t newsize) noexcept {
            m_Size = newsize;
        }

        void clear() noexcept {
            m_Size = 0;
        }

        T& operator[](size_t idx) {
            return m_Data[idx];
        }

        T const& operator[](size_t i) const noexcept {
            return m_Data[i];
        }

    private:
        std::array<T, N> m_Data{};
        size_t m_Size = 0;
    };

    using ComponentPath = SizedArray<OpenSim::Component const*, 16>;

    // populates `out` with the sequence of nodes between (ancestor..child]
    void computeComponentPath(
        OpenSim::Component const* ancestor,
        OpenSim::Component const* child,
        ComponentPath& out)
    {

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

    bool pathContains(ComponentPath const& p, OpenSim::Component const* c)
    {
        auto end = p.begin() == p.end() ? p.end() : p.end()-1;
        return std::find(p.begin(), end, c) != end;
    }

    enum class ResponseType {
        NothingHappened,
        SelectionChanged,
        HoverChanged,
    };

    struct Response final {
        OpenSim::Component const* ptr = nullptr;
        ResponseType type = ResponseType::NothingHappened;
    };

    bool isSearchHit(std::string const& searchStr, ComponentPath const& cp)
    {
        return std::any_of(cp.begin(), cp.end(), [&searchStr](OpenSim::Component const* c)
        {
            return osc::ContainsSubstringCaseInsensitive(c->getName(), searchStr);
        });
    }
}

class osc::NavigatorPanel::Impl final : public StandardPanel {
public:
    Impl(
        std::string_view panelName,
        std::shared_ptr<VirtualModelStatePair> model,
        std::function<void(OpenSim::ComponentPath const&)> onRightClick) :

        StandardPanel{panelName},
        m_Model{std::move(model)},
        m_OnRightClick{std::move(onRightClick)}
    {
    }

    bool isOpen() const
    {
        return static_cast<StandardPanel const&>(*this).isOpen();
    }

    void open()
    {
        return static_cast<StandardPanel&>(*this).open();
    }

    void close()
    {
        return static_cast<StandardPanel&>(*this).close();
    }

private:

    void implDrawContent() final
    {
        if (!m_Model)
        {
            ImGui::TextDisabled("(no model)");
            return;
        }

        Response r = drawWithResponse();
        if (r.type == ResponseType::SelectionChanged)
        {
            m_Model->setSelected(r.ptr);
        }
        else if (r.type == ResponseType::HoverChanged)
        {
            m_Model->setHovered(r.ptr);
        }
    }

    Response drawWithResponse()
    {
        Response rv;

        ImGui::Dummy({0.0f, 3.0f});

        // draw filter stuff

        ImGui::TextUnformatted(ICON_FA_EYE);
        if (ImGui::BeginPopupContextItem("##filterpopup"))
        {
            ImGui::Checkbox("frames", &m_ShowFrames);
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        osc::DrawSearchBar(m_CurrentSearch);

        ImGui::Dummy({0.0f, 3.0f});
        ImGui::Separator();
        ImGui::Dummy({0.0f, 3.0f});

        // draw content
        ImGui::BeginChild("##componentnavigatorvieweritems", {0.0, 0.0}, false, ImGuiWindowFlags_NoBackground);

        OpenSim::Component const* root = &m_Model->getModel();
        OpenSim::Component const* selection = m_Model->getSelected();
        OpenSim::Component const* hover = m_Model->getHovered();

        ComponentPath selectionPath{};
        if (selection)
        {
            computeComponentPath(root, selection, selectionPath);
        }

        ComponentPath hoverPath{};
        if (hover)
        {
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
        bool const hasSearch = !m_CurrentSearch.empty();

        float const unindentPerLevel = ImGui::GetTreeNodeToLabelSpacing() - 15.0f;

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
                if (!m_ShowFrames && hc == typeid(OpenSim::FrameGeometry).hash_code())
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

            bool const searchHit = hasSearch && isSearchHit(m_CurrentSearch, currentPath);

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
            bool const isInternalNode = currentPath.size() < 2 || lookaheadPath.size() > currentPath.size();
            ImGuiTreeNodeFlags const nodeFlags = isInternalNode ? ImGuiTreeNodeFlags_OpenOnArrow : (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet);

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
                rv.type = ResponseType::HoverChanged;
                rv.ptr = cur;

                osc::DrawTooltip(cur->getConcreteClassName());
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                rv.type = ResponseType::SelectionChanged;
                rv.ptr = cur;
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                m_OnRightClick(osc::GetAbsolutePath(*cur));
            }
        }

        // pop remaining dangling tree elements
        while (imguiTreeDepth-- > 0)
        {
            ImGui::Indent(unindentPerLevel);
            ImGui::TreePop();
        }

        ImGui::EndChild();

        return rv;
    }

    std::shared_ptr<VirtualModelStatePair> m_Model;
    std::function<void(OpenSim::ComponentPath const&)> m_OnRightClick;
    std::string m_CurrentSearch;
    bool m_ShowFrames = false;
};


// public API (PIMPL)

osc::NavigatorPanel::NavigatorPanel(
    std::string_view panelName,
    std::shared_ptr<VirtualModelStatePair> model,
    std::function<void(OpenSim::ComponentPath const&)> onRightClick) :

    m_Impl{std::make_unique<Impl>(panelName, std::move(model), std::move(onRightClick))}
{
}

osc::NavigatorPanel::NavigatorPanel(NavigatorPanel&&) noexcept = default;
osc::NavigatorPanel& osc::NavigatorPanel::operator=(NavigatorPanel&&) noexcept = default;
osc::NavigatorPanel::~NavigatorPanel() noexcept = default;

osc::CStringView osc::NavigatorPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::NavigatorPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::NavigatorPanel::implOpen()
{
    m_Impl->open();
}

void osc::NavigatorPanel::implClose()
{
    m_Impl->close();
}

void osc::NavigatorPanel::implDraw()
{
    m_Impl->draw();
}
