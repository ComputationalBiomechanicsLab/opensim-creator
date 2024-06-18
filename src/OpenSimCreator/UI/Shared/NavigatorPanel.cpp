#include "NavigatorPanel.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/UI/ImGuiHelpers.h>
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

    using ComponentPath = SizedArray<const OpenSim::Component*, 16>;

    // populates `out` with the sequence of nodes between (ancestor..child]
    void computeComponentPath(
        const OpenSim::Component* ancestor,
        const OpenSim::Component* child,
        ComponentPath& out)
    {

        out.clear();

        // populate child --> parent
        for (; child != nullptr; child = GetOwner(*child))
        {
            out.push_back(child);

            if (!child->hasOwner() || child == ancestor)
            {
                break;
            }
        }

        // reverse to yield parent --> child
        rgs::reverse(out);
    }

    bool pathContains(const ComponentPath& p, const OpenSim::Component* c)
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

    bool isSearchHit(const std::string& searchStr, const ComponentPath& cp)
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
    {
    }

    bool isOpen() const
    {
        return static_cast<const StandardPanelImpl&>(*this).is_open();
    }

    void open()
    {
        return static_cast<StandardPanelImpl&>(*this).open();
    }

    void close()
    {
        return static_cast<StandardPanelImpl&>(*this).close();
    }

private:

    void impl_draw_content() final
    {
        if (!m_Model)
        {
            ui::draw_text_disabled("(no model)");
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

        ui::draw_dummy({0.0f, 3.0f});

        // draw filter stuff

        ui::draw_text_unformatted(ICON_FA_EYE);
        if (ui::begin_popup_context_menu("##filterpopup"))
        {
            ui::draw_checkbox("frames", &m_ShowFrames);
            ui::end_popup();
        }
        ui::same_line();
        DrawSearchBar(m_CurrentSearch);

        ui::draw_dummy({0.0f, 3.0f});
        ui::draw_separator();
        ui::draw_dummy({0.0f, 3.0f});

        // draw content
        ui::begin_child_panel("##componentnavigatorvieweritems", {0.0, 0.0}, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);

        const OpenSim::Component* root = &m_Model->getModel();
        const OpenSim::Component* selection = m_Model->getSelected();
        const OpenSim::Component* hover = m_Model->getHovered();

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
        const auto lst = root->getComponentList();
        auto it = lst.begin();
        const auto end = lst.end();

        // initially populate lookahead (+ path)
        const OpenSim::Component* lookahead = root;
        ComponentPath lookaheadPath;
        computeComponentPath(root, root, lookaheadPath);

        // set cur path empty (first step copies lookahead into this)
        const OpenSim::Component* cur = nullptr;
        ComponentPath currentPath;

        int imguiTreeDepth = 0;
        int imguiId = 0;
        const bool hasSearch = !m_CurrentSearch.empty();

        const float unindentPerLevel = ui::get_tree_node_to_label_spacing() - 15.0f;

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
                const OpenSim::Component& c = *it++;

                bool shouldRender = true;

                if (!m_ShowFrames && dynamic_cast<const OpenSim::FrameGeometry*>(&c))
                {
                    shouldRender = false;
                }
                else if (const auto* wos = dynamic_cast<const OpenSim::WrapObjectSet*>(&c))
                {
                    shouldRender = !empty(*wos);
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

            const bool searchHit = hasSearch && isSearchHit(m_CurrentSearch, currentPath);

            // skip rendering if a parent node is collapsed
            if (imguiTreeDepth < currentPath.sizei() - 1)
            {
                continue;
            }

            // pop tree nodes down to the current depth
            while (imguiTreeDepth >= currentPath.sizei())
            {
                ui::indent(unindentPerLevel);
                ui::tree_pop();
                --imguiTreeDepth;
            }
            OSC_ASSERT(imguiTreeDepth <= currentPath.sizei() - 1);

            // handle display mode (node vs leaf)
            const bool isInternalNode = currentPath.size() < 2 || lookaheadPath.size() > currentPath.size();
            const ImGuiTreeNodeFlags nodeFlags = isInternalNode ? ImGuiTreeNodeFlags_OpenOnArrow : (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet);

            // handle coloring
            int styles = 0;
            if (cur == selection)
            {
                ui::push_style_color(ImGuiCol_Text, Color::yellow());
                ++styles;
            }
            else if (cur == hover)
            {
                ui::push_style_color(ImGuiCol_Text, Color::yellow());
                ++styles;
            }
            else if (!hasSearch || searchHit)
            {
                // display as normal
            }
            else
            {
                ui::push_style_color(ImGuiCol_Text, Color::half_grey());
                ++styles;
            }

            // auto-open in these cases
            if (searchHit || currentPath.sizei() == 1 || pathContains(selectionPath, cur))
            {
                ui::set_next_item_open(true);
            }

            ui::push_id(imguiId);
            if (ui::draw_tree_node_ex(cur->getName(), nodeFlags))
            {
                ui::unindent(unindentPerLevel);
                ++imguiTreeDepth;
            }
            ui::pop_id();
            ui::pop_style_color(styles);

            if (ui::is_item_hovered())
            {
                rv.type = ResponseType::HoverChanged;
                rv.ptr = cur;

                ui::draw_tooltip(cur->getConcreteClassName());
            }

            if (ui::is_item_clicked(ImGuiMouseButton_Left))
            {
                rv.type = ResponseType::SelectionChanged;
                rv.ptr = cur;
            }

            if (ui::is_item_clicked(ImGuiMouseButton_Right))
            {
                m_OnRightClick(GetAbsolutePath(*cur));
            }
        }

        // pop remaining dangling tree elements
        while (imguiTreeDepth-- > 0)
        {
            ui::indent(unindentPerLevel);
            ui::tree_pop();
        }

        ui::end_child_panel();

        return rv;
    }

    std::shared_ptr<IModelStatePair> m_Model;
    std::function<void(const OpenSim::ComponentPath&)> m_OnRightClick;
    std::string m_CurrentSearch;
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
    return m_Impl->isOpen();
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
