#include "NavigatorPanel.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/Platform/OSCColors.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/StringHelpers.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    // poor-man's abstraction for a constant-sized array
    template<typename T, size_t N>
    class SizedArray final {
        static_assert(std::is_trivial_v<T>);
        static_assert(N <= std::numeric_limits<int>::max());

    public:
        void push_back(T v)
        {
            if (m_Size >= N) {
                throw std::runtime_error{"cannot render a navigator: the Model/Component tree is too deep"};
            }
            m_Data[m_Size++] = v;
        }

        const T* begin() const
        {
            return m_Data.data();
        }

        T* begin()
        {
            return m_Data.data();
        }

        const T* end() const
        {
            return m_Data.data() + m_Size;
        }

        T* end()
        {
            return m_Data.data() + m_Size;
        }

        size_t size() const
        {
            return m_Size;
        }

        ptrdiff_t sizei() const
        {
            return static_cast<ptrdiff_t>(m_Size);
        }

        bool empty() const
        {
            return m_Size == 0;
        }

        void resize(size_t newsize)
        {
            m_Size = newsize;
        }

        void clear()
        {
            m_Size = 0;
        }

        T& operator[](size_t idx)
        {
            return m_Data[idx];
        }

        const T& operator[](size_t i) const
        {
            return m_Data[i];
        }

    private:
        std::array<T, N> m_Data{};
        size_t m_Size = 0;
    };

    using ComponentTreePathPointers = SizedArray<const OpenSim::Component*, 16>;

    // populates `out` with the sequence of nodes between (ancestor..child]
    ComponentTreePathPointers computeComponentTreePath(
        const OpenSim::Component* ancestor,
        const OpenSim::Component* child)
    {
        ComponentTreePathPointers rv;

        // populate child --> parent
        for (; child != nullptr; child = GetOwner(*child)) {
            rv.push_back(child);

            if (!child->hasOwner() || child == ancestor) {
                break;
            }
        }

        // reverse to yield parent --> child
        rgs::reverse(rv);

        return rv;
    }

    bool pathContains(const ComponentTreePathPointers& p, const OpenSim::Component* c)
    {
        auto end = p.begin() == p.end() ? p.end() : p.end()-1;
        return cpp23::contains(p.begin(), end, c);
    }

    enum class ResponseType {
        NothingHappened,
        SelectionChanged,
        HoverChanged,
    };

    struct Response final {
        const OpenSim::Component* ptr = nullptr;
        ResponseType type = ResponseType::NothingHappened;
    };

    bool isSearchHit(const std::string& searchStr, const ComponentTreePathPointers& cp)
    {
        return rgs::any_of(cp, [&searchStr](const OpenSim::Component* c)
        {
            return contains_case_insensitive(c->getName(), searchStr);
        });
    }
}

class osc::NavigatorPanel::Impl final : public StandardPanelImpl {
public:
    Impl(
        std::string_view panelName,
        std::shared_ptr<IModelStatePair> model,
        std::function<void(const OpenSim::ComponentPath&)> onRightClick) :

        StandardPanelImpl{panelName},
        m_Model{std::move(model)},
        m_OnRightClick{std::move(onRightClick)}
    {}

private:

    void impl_draw_content() final
    {
        if (not m_Model) {
            ui::draw_text_disabled("(no model)");  // edge-case
            return;
        }

        // draw the UI
        const Response response = drawWithResponse();

        // propagate any UI-initated changes
        if (response.type == ResponseType::SelectionChanged) {
            m_Model->setSelected(response.ptr);
        }
        else if (response.type == ResponseType::HoverChanged) {
            m_Model->setHovered(response.ptr);
        }
    }

    Response drawWithResponse()
    {
        Response rv;
        ui::draw_dummy({0.0f, 3.0f});
        drawFilterAndSearchRow();
        drawNavigationTreeChildPanel(rv);
        return rv;
    }

    void drawFilterAndSearchRow()
    {
        ui::draw_text_unformatted(ICON_FA_EYE);
        if (ui::begin_popup_context_menu("##filterpopup")) {
            ui::draw_checkbox("frames", &m_ShowFrames);
            ui::end_popup();
        }
        ui::same_line();
        DrawSearchBar(m_CurrentSearch);

        ui::draw_dummy({0.0f, 3.0f});
        ui::draw_separator();
        ui::draw_dummy({0.0f, 3.0f});
    }

    void drawNavigationTreeChildPanel(Response& rv)
    {
        ui::begin_child_panel(
            "##componentnavigatorvieweritems",
            Vec2{0.0, 0.0},
            ImGuiChildFlags_None,
            ui::WindowFlag::NoBackground
        );

        drawNavigationTreeContent(rv);

        ui::end_child_panel();
    }

    void drawNavigationTreeContent(Response& rv)
    {
        // these remain constant when rendering the tree
        const bool hasSearch = not m_CurrentSearch.empty();
        const float unindentPerLevel = ui::get_tree_node_to_label_spacing() - 15.0f;

        const OpenSim::Component* root = &m_Model->getModel();
        const OpenSim::Component* selected = m_Model->getSelected();
        const OpenSim::Component* hovered = m_Model->getHovered();

        OpenSim::ComponentPath selectedPath = GetAbsolutePathOrEmpty(selected);

        const ComponentTreePathPointers selectedPathPointers = selected ?
            computeComponentTreePath(root, selected) :
            ComponentTreePathPointers{};

        // get underlying component list (+iterator)
        const auto componentList = root->getComponentList();
        const auto componentListEnd = componentList.end();

        // setup loop invariants
        auto componentListIterator = componentList.begin();
        const OpenSim::Component* lookahead = root;
        ComponentTreePathPointers lookaheadPath = computeComponentTreePath(root, root);
        int imguiTreeDepth = 0;
        int imguiId = 0;

        while (lookahead) {

            // important: ensure all nodes have a unique ID: regardess of filtering
            ++imguiId;

            // populate current (+ path) from lookahead
            const OpenSim::Component* cur = lookahead;
            ComponentTreePathPointers currentPath = lookaheadPath;

            OSC_ASSERT(cur && "cur ptr should *definitely* be populated at this point");
            OSC_ASSERT(!currentPath.empty() && "current path cannot be empty (even a root element has a path)");

            // update lookahead (+ path) by stepping to the next component in the component tree
            lookahead = nullptr;
            lookaheadPath.clear();
            while (componentListIterator != componentListEnd) {
                const OpenSim::Component& c = *componentListIterator++;

                bool shouldRender = true;

                if (!m_ShowFrames && dynamic_cast<const OpenSim::FrameGeometry*>(&c)) {
                    shouldRender = false;
                }
                else if (const auto* wos = dynamic_cast<const OpenSim::WrapObjectSet*>(&c)) {
                    shouldRender = !empty(*wos);
                }
                else if (!ShouldShowInUI(c)) {
                    shouldRender = false;
                }

                if (shouldRender) {
                    lookahead = &c;
                    lookaheadPath = computeComponentTreePath(root, &c);
                    break;
                }
            }
            OSC_ASSERT((lookahead || !lookahead) && "a lookahead is not *required* at this point");

            const bool searchHit = hasSearch && isSearchHit(m_CurrentSearch, currentPath);

            // skip rendering if a parent node is collapsed
            if (imguiTreeDepth < currentPath.sizei() - 1) {
                continue;
            }

            // pop tree nodes down to the current depth
            while (imguiTreeDepth >= currentPath.sizei()) {
                ui::indent(unindentPerLevel);
                ui::tree_pop();
                --imguiTreeDepth;
            }
            OSC_ASSERT(imguiTreeDepth <= currentPath.sizei() - 1);

            // handle display mode (node vs leaf)
            const bool isInternalNode = currentPath.size() < 2 || lookaheadPath.size() > currentPath.size();
            const ui::TreeNodeFlags nodeFlags = isInternalNode ? ui::TreeNodeFlag::OpenOnArrow : ui::TreeNodeFlags{ui::TreeNodeFlag::Leaf, ui::TreeNodeFlag::Bullet};

            // handle coloring
            int pushedStyles = 0;
            if (cur == selected) {
                ui::push_style_color(ImGuiCol_Text, OSCColors::selected());
                ++pushedStyles;
            }
            else if (cur == hovered) {
                ui::push_style_color(ImGuiCol_Text, OSCColors::hovered());
                ++pushedStyles;
            }
            else if (!hasSearch || searchHit) {
                // display as normal
            }
            else {
                ui::push_style_color(ImGuiCol_Text, OSCColors::disabled());
                ++pushedStyles;
            }

            // auto-open in these cases
            if (searchHit || currentPath.sizei() == 1 || pathContains(selectedPathPointers, cur)) {
                ui::set_next_item_open(true);
            }

            // draw the tree leaf/node
            ui::push_id(imguiId);
            if (ui::draw_tree_node_ex(cur->getName(), nodeFlags)) {
                ui::unindent(unindentPerLevel);
                ++imguiTreeDepth;
            }
            ui::pop_id();
            ui::pop_style_color(pushedStyles);

            // handle tree node user interaction
            const bool userHoveringThisTreeNode = ui::is_item_hovered();
            const bool userLeftClickedThisTreeNode = ui::is_item_clicked(ui::MouseButton::Left);
            const bool userRightClickedThisTreeNode = ui::is_item_clicked(ui::MouseButton::Right);

            if (userHoveringThisTreeNode) {
                rv.type = ResponseType::HoverChanged;
                rv.ptr = cur;
                ui::draw_tooltip(cur->getConcreteClassName());
            }
            if (userLeftClickedThisTreeNode) {
                rv.type = ResponseType::SelectionChanged;
                rv.ptr = cur;
            }
            if (userRightClickedThisTreeNode) {
                m_OnRightClick(GetAbsolutePath(*cur));
            }
            if (cur == selected and
                selectedPath != m_PreviousSelectionPath and
                not userLeftClickedThisTreeNode) {

                // if the current tree element being drawn is also the current
                // selection, and the selection differs from the previous
                // selection, then automatically scroll to this tree node (#908)
                ui::set_scroll_y_here();
            }
        }

        // pop remaining dangling tree elements
        while (imguiTreeDepth-- > 0) {
            ui::indent(unindentPerLevel);
            ui::tree_pop();
        }

        // cache the previous selection path, so we can observe when it has changed (#908)
        m_PreviousSelectionPath = std::move(selectedPath);
    }

    std::shared_ptr<IModelStatePair> m_Model;
    std::function<void(const OpenSim::ComponentPath&)> m_OnRightClick;
    std::string m_CurrentSearch;
    OpenSim::ComponentPath m_PreviousSelectionPath;
    bool m_ShowFrames = false;
};


osc::NavigatorPanel::NavigatorPanel(
    std::string_view panelName,
    std::shared_ptr<IModelStatePair> model,
    std::function<void(const OpenSim::ComponentPath&)> onRightClick) :

    m_Impl{std::make_unique<Impl>(panelName, std::move(model), std::move(onRightClick))}
{}
osc::NavigatorPanel::NavigatorPanel(NavigatorPanel&&) noexcept = default;
osc::NavigatorPanel& osc::NavigatorPanel::operator=(NavigatorPanel&&) noexcept = default;
osc::NavigatorPanel::~NavigatorPanel() noexcept = default;

CStringView osc::NavigatorPanel::impl_get_name() const
{
    return m_Impl->name();
}

bool osc::NavigatorPanel::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::NavigatorPanel::impl_open()
{
    m_Impl->open();
}

void osc::NavigatorPanel::impl_close()
{
    m_Impl->close();
}

void osc::NavigatorPanel::impl_on_draw()
{
    m_Impl->on_draw();
}
