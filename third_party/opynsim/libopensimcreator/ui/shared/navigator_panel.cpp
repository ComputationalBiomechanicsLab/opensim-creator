#include "navigator_panel.h"

#include <libopensimcreator/platform/open_sim_creator_colors.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/color.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/string_helpers.h>
#include <liboscar/utilities/variable_length_array.h>
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
    ComponentTreePathPointers compute_component_tree_path(
        const OpenSim::Component* ancestor, const OpenSim::Component* child)
    {
        ComponentTreePathPointers rv;

        // populate child --> parent
        for (; child != nullptr; child = opyn::get_owner(*child)) {
            rv.push_back(child);

            if (!child->hasOwner() || child == ancestor) {
                break;
            }
        }

        // reverse to yield parent --> child
        rgs::reverse(rv);

        return rv;
    }

    bool path_contains(const ComponentTreePathPointers& p, const OpenSim::Component* c)
    {
        auto end = p.begin() == p.end() ? p.end() : p.end() - 1;
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

    bool is_search_hit(const std::string& search_str, const ComponentTreePathPointers& cp)
    {
        return rgs::any_of(cp, [&search_str](const OpenSim::Component* c) {
            return contains_case_insensitive(c->getName(), search_str);
        });
    }
}

class osc::NavigatorPanel::Impl final : public PanelPrivate {
public:
    Impl(
        NavigatorPanel& owner,
        Widget* parent,
        std::string_view panel_name,
        std::shared_ptr<opyn::ModelStatePair> model,
        std::function<void(const OpenSim::ComponentPath&)> on_right_click) :

        PanelPrivate{owner, parent, panel_name},
        model_{std::move(model)},
        on_right_click_{std::move(on_right_click)}
    {}

    void draw_content()
    {
        if (not model_) {
            ui::draw_text_disabled("(no model)");  // edge-case
            return;
        }

        // draw the UI
        const Response response = draw_with_response();

        // propagate any UI-initated changes
        if (response.type == ResponseType::SelectionChanged) {
            model_->set_selected(response.ptr);
        } else if (response.type == ResponseType::HoverChanged) {
            model_->set_hovered(response.ptr);
        }
    }

private:
    Response draw_with_response()
    {
        Response rv;
        draw_filter_and_search_row();
        ui::draw_vertical_spacer(0.1f);
        ui::draw_separator();
        draw_navigation_tree_child_panel(rv);
        return rv;
    }

    void draw_filter_and_search_row()
    {
        ui::set_next_item_width(ui::get_content_region_available().x());
        draw_search_bar(current_search_);
    }

    void draw_navigation_tree_child_panel(Response& rv)
    {
        ui::begin_child_panel(
            "##componentnavigatorvieweritems",
            Vector2{0.0, 0.0},
            ui::ChildPanelFlags{},
            ui::PanelFlag::NoBackground);

        ui::draw_vertical_spacer(0.05f);
        draw_navigation_tree_content(rv);

        ui::end_child_panel();
    }

    void draw_navigation_tree_content(Response& rv)
    {
        // these remain constant when rendering the tree
        const bool has_search = not current_search_.empty();
        const float unindent_per_level = ui::get_tree_node_to_label_spacing() - 15.0f;

        const OpenSim::Component* root = &model_->get_model();
        const OpenSim::Component* selected = model_->get_selected();
        const OpenSim::Component* hovered = model_->get_hovered();

        OpenSim::ComponentPath selected_path = opyn::get_absolute_path_or_empty(selected);

        const ComponentTreePathPointers selected_path_pointers =
            selected ? compute_component_tree_path(root, selected) : ComponentTreePathPointers{};

        // get underlying component list (+iterator)
        const auto component_list = root->getComponentList();
        const auto component_list_end = component_list.end();

        // setup loop invariants
        auto component_list_iterator = component_list.begin();
        const OpenSim::Component* lookahead = root;
        ComponentTreePathPointers lookahead_path = compute_component_tree_path(root, root);
        int imgui_tree_depth = 0;
        int imgui_id = 0;
        int row = 0;

        while (lookahead) {

            // important: ensure all nodes have a unique ID: regardess of filtering
            ++imgui_id;

            // populate current (+ path) from lookahead
            const OpenSim::Component* cur = lookahead;
            ComponentTreePathPointers current_path = lookahead_path;

            OSC_ASSERT(cur && "cur ptr should *definitely* be populated at this point");
            OSC_ASSERT(
                !current_path.empty() &&
                "current path cannot be empty (even a root element has a path)");

            // update lookahead (+ path) by stepping to the next component in the component tree
            lookahead = nullptr;
            lookahead_path.clear();
            while (component_list_iterator != component_list_end) {
                const OpenSim::Component& c = *component_list_iterator++;

                bool should_render = true;

                if (not show_frames_ and dynamic_cast<const OpenSim::FrameGeometry*>(&c)) {
                    should_render = false;
                } else if (const auto* wos = dynamic_cast<const OpenSim::WrapObjectSet*>(&c)) {
                    should_render = !opyn::empty(*wos);
                } else if (not opyn::should_show_in_ui(c)) {
                    should_render = false;
                }

                if (should_render) {
                    lookahead = &c;
                    lookahead_path = compute_component_tree_path(root, &c);
                    break;
                }
            }
            OSC_ASSERT((lookahead || !lookahead) && "a lookahead is not *required* at this point");

            const bool search_hit = has_search && is_search_hit(current_search_, current_path);

            // skip rendering if a parent node is collapsed
            if (imgui_tree_depth < std::ssize(current_path) - 1) {
                continue;
            }

            // pop tree nodes down to the current depth
            while (imgui_tree_depth >= std::ssize(current_path)) {
                --imgui_tree_depth;
                ui::indent(unindent_per_level);
                ui::tree_pop();
                ui::pop_id();
            }
            OSC_ASSERT(imgui_tree_depth <= std::ssize(current_path) - 1);

            // handle display mode (node vs leaf)
            const bool is_internal_node =
                current_path.size() < 2 || lookahead_path.size() > current_path.size();
            ui::TreeNodeFlags node_flags = ui::TreeNodeFlag::DrawLinesFull;
            node_flags |= is_internal_node ? ui::TreeNodeFlag::OpenOnArrow : ui::TreeNodeFlag::Leaf;

            // handle alternating background colors
            if (row++ % 2) {
                const auto offset = ui::get_cursor_ui_position() - ui::get_cursor_panel_position();
                const auto top_left = Vector2{0.0f, ui::get_cursor_panel_position().y()};
                const auto bottom_right =
                    top_left + Vector2{
                                   ui::get_panel_size().x(),
                                   ui::get_text_line_height_with_spacing_in_current_panel()};
                const auto rect = Rect::from_corners(offset + top_left, offset + bottom_right);
                const auto color = multiply_luminance(ui::get_color(ui::ColorVar::PanelBg), 1.2f);
                ui::get_panel_draw_list().add_rect_filled(rect, color);
            }

            // handle coloring
            int pushed_styles = 0;
            if (cur == selected) {
                ui::push_style_color(ui::ColorVar::Text, OSCColors::selected());
                ++pushed_styles;
            } else if (cur == hovered) {
                ui::push_style_color(ui::ColorVar::Text, OSCColors::hovered());
                ++pushed_styles;
            } else if (not has_search or search_hit) {
                // display as normal
            } else {
                ui::push_style_color(ui::ColorVar::Text, OSCColors::disabled());
                ++pushed_styles;
            }

            // auto-open in these cases
            if (search_hit or std::ssize(current_path) == 1 or
                path_contains(selected_path_pointers, cur)) {

                ui::set_next_item_open(true);
            }

            // draw the tree leaf/node
            ui::push_id(imgui_id);
            std::stringstream ss;
            ss << icon_for(*cur) << ' ' << cur->getName();
            if (ui::draw_tree_node_ex(std::move(ss).str(), node_flags)) {
                ui::unindent(unindent_per_level);
                ++imgui_tree_depth;
            } else {
                ui::pop_id();
            }
            ui::pop_style_color(pushed_styles);

            // handle tree node user interaction
            const bool user_hovering_this_tree_node = ui::is_item_hovered();
            const bool user_left_clicked_this_tree_node = ui::is_item_clicked(MouseButton::Left);
            const bool user_right_clicked_this_tree_node = ui::is_item_clicked(MouseButton::Right);

            if (user_hovering_this_tree_node) {
                rv.type = ResponseType::HoverChanged;
                rv.ptr = cur;
                draw_component_hover_tooltip(*cur);
            }
            if (user_left_clicked_this_tree_node) {
                rv.type = ResponseType::SelectionChanged;
                rv.ptr = cur;
            }
            if (user_right_clicked_this_tree_node) {
                on_right_click_(opyn::get_absolute_path(*cur));
            }
            if (cur == selected and selected_path != previous_selection_path_ and
                not user_left_clicked_this_tree_node) {

                // if the current tree element being drawn is also the current
                // selection, and the selection differs from the previous
                // selection, then automatically scroll to this tree node (#908)
                ui::set_scroll_y_here();
            }
        }

        // pop remaining dangling tree elements
        while (imgui_tree_depth-- > 0) {
            ui::indent(unindent_per_level);
            ui::tree_pop();
            ui::pop_id();
        }

        // cache the previous selection path, so we can observe when it has changed (#908)
        if (rv.type == ResponseType::SelectionChanged) {
            previous_selection_path_ = opyn::get_absolute_path_or_empty(rv.ptr);
        } else {
            previous_selection_path_ = std::move(selected_path);
        }
    }

    std::shared_ptr<opyn::ModelStatePair> model_;
    std::function<void(const OpenSim::ComponentPath&)> on_right_click_;
    std::string current_search_;
    OpenSim::ComponentPath previous_selection_path_;
    bool show_frames_ = false;
};

osc::NavigatorPanel::NavigatorPanel(
    Widget* parent,
    std::string_view panel_name,
    std::shared_ptr<opyn::ModelStatePair> model,
    std::function<void(const OpenSim::ComponentPath&)> on_right_click) :

    Panel{std::make_unique<Impl>(
        *this, parent, panel_name, std::move(model), std::move(on_right_click))}
{}
void osc::NavigatorPanel::impl_draw_content()
{
    private_data().draw_content();
}
