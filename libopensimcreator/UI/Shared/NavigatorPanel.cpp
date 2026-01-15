#include "NavigatorPanel.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Platform/OSCColors.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/graphics/Color.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/utils/Assertions.h>
#include <liboscar/utils/StringHelpers.h>
#include <liboscar/utils/VariableLengthArray.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    using ComponentTreePathPointers = VariableLengthArray<const OpenSim::Component*, 8>;

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

class osc::NavigatorPanel::Impl final : public PanelPrivate {
public:
    Impl(
        NavigatorPanel& owner,
        Widget* parent,
        std::string_view panelName,
        std::shared_ptr<IModelStatePair> model,
        std::function<void(const OpenSim::ComponentPath&)> onRightClick) :

        PanelPrivate{owner, parent, panelName},
        m_Model{std::move(model)},
        m_OnRightClick{std::move(onRightClick)}
    {}

    void draw_content()
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

private:
    Response drawWithResponse()
    {
        Response rv;
        drawFilterAndSearchRow();
        ui::draw_vertical_spacer(0.1f);
        ui::draw_separator();
        drawNavigationTreeChildPanel(rv);
        return rv;
    }

    void drawFilterAndSearchRow()
    {
        ui::set_next_item_width(ui::get_content_region_available().x);
        DrawSearchBar(m_CurrentSearch);
    }

    void drawNavigationTreeChildPanel(Response& rv)
    {
        ui::begin_child_panel(
            "##componentnavigatorvieweritems",
            Vector2{0.0, 0.0},
            ui::ChildPanelFlags{},
            ui::PanelFlag::NoBackground
        );

        ui::draw_vertical_spacer(0.05f);
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
        int row = 0;

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
            if (imguiTreeDepth < std::ssize(currentPath) - 1) {
                continue;
            }

            // pop tree nodes down to the current depth
            while (imguiTreeDepth >= std::ssize(currentPath)) {
                --imguiTreeDepth;
                ui::indent(unindentPerLevel);
                ui::tree_pop();
                ui::pop_id();
            }
            OSC_ASSERT(imguiTreeDepth <= std::ssize(currentPath) - 1);

            // handle display mode (node vs leaf)
            const bool isInternalNode = currentPath.size() < 2 || lookaheadPath.size() > currentPath.size();
            ui::TreeNodeFlags nodeFlags = ui::TreeNodeFlag::DrawLinesFull;
            nodeFlags |= isInternalNode ? ui::TreeNodeFlag::OpenOnArrow : ui::TreeNodeFlag::Leaf;

            // handle alternating background colors
            if (row++ % 2) {
                const auto offset = ui::get_cursor_ui_position() - ui::get_cursor_panel_position();
                const auto topLeft = Vector2{0.0f, ui::get_cursor_panel_position().y};
                const auto bottomRight =  topLeft + Vector2{ui::get_panel_size().x, ui::get_text_line_height_with_spacing_in_current_panel()};
                const auto rect = Rect::from_corners(offset + topLeft, offset + bottomRight);
                const auto color = multiply_luminance(ui::get_color(ui::ColorVar::PanelBg), 1.2f);
                ui::get_panel_draw_list().add_rect_filled(rect, color);
            }

            // handle coloring
            int pushedStyles = 0;
            if (cur == selected) {
                ui::push_style_color(ui::ColorVar::Text, OSCColors::selected());
                ++pushedStyles;
            }
            else if (cur == hovered) {
                ui::push_style_color(ui::ColorVar::Text, OSCColors::hovered());
                ++pushedStyles;
            }
            else if (!hasSearch || searchHit) {
                // display as normal
            }
            else {
                ui::push_style_color(ui::ColorVar::Text, OSCColors::disabled());
                ++pushedStyles;
            }

            // auto-open in these cases
            if (searchHit || std::ssize(currentPath) == 1 || pathContains(selectedPathPointers, cur)) {
                ui::set_next_item_open(true);
            }

            // draw the tree leaf/node
            ui::push_id(imguiId);
            std::stringstream ss;
            ss << IconFor(*cur) << ' ' << cur->getName();
            if (ui::draw_tree_node_ex(std::move(ss).str(), nodeFlags)) {
                ui::unindent(unindentPerLevel);
                ++imguiTreeDepth;
            }
            else {
                ui::pop_id();
            }
            ui::pop_style_color(pushedStyles);

            // handle tree node user interaction
            const bool userHoveringThisTreeNode = ui::is_item_hovered();
            const bool userLeftClickedThisTreeNode = ui::is_item_clicked(ui::MouseButton::Left);
            const bool userRightClickedThisTreeNode = ui::is_item_clicked(ui::MouseButton::Right);

            if (userHoveringThisTreeNode) {
                rv.type = ResponseType::HoverChanged;
                rv.ptr = cur;
                DrawComponentHoverTooltip(*cur);
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
            ui::pop_id();
        }

        // cache the previous selection path, so we can observe when it has changed (#908)
        if (rv.type == ResponseType::SelectionChanged) {
            m_PreviousSelectionPath = GetAbsolutePathOrEmpty(rv.ptr);
        }
        else {
            m_PreviousSelectionPath = std::move(selectedPath);
        }
    }

    std::shared_ptr<IModelStatePair> m_Model;
    std::function<void(const OpenSim::ComponentPath&)> m_OnRightClick;
    std::string m_CurrentSearch;
    OpenSim::ComponentPath m_PreviousSelectionPath;
    bool m_ShowFrames = false;
};

osc::NavigatorPanel::NavigatorPanel(
    Widget* parent,
    std::string_view panelName,
    std::shared_ptr<IModelStatePair> model,
    std::function<void(const OpenSim::ComponentPath&)> onRightClick) :

    Panel{std::make_unique<Impl>(*this, parent, panelName, std::move(model), std::move(onRightClick))}
{}
void osc::NavigatorPanel::impl_draw_content() { private_data().draw_content(); }
