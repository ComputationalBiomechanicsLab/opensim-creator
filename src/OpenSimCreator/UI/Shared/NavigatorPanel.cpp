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
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>

using namespace osc;

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

        T const* begin() const {
            return m_Data.data();
        }

        T* begin() {
            return m_Data.data();
        }

        T const* end() const {
            return m_Data.data() + m_Size;
        }

        T* end() {
            return m_Data.data() + m_Size;
        }

        size_t size() const {
            return m_Size;
        }

        ptrdiff_t sizei() const {
            return static_cast<ptrdiff_t>(m_Size);
        }

        bool empty() const {
            return m_Size == 0;
        }

        void resize(size_t newsize) {
            m_Size = newsize;
        }

        void clear() {
            m_Size = 0;
        }

        T& operator[](size_t idx) {
            return m_Data[idx];
        }

        T const& operator[](size_t i) const {
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
        for (; child != nullptr; child = GetOwner(*child))
        {
            out.push_back(child);

            if (!child->hasOwner() || child == ancestor)
            {
                break;
            }
        }

        // reverse to yield parent --> child
        reverse(out);
    }

    bool pathContains(ComponentPath const& p, OpenSim::Component const* c)
    {
        auto end = p.begin() == p.end() ? p.end() : p.end()-1;
        return contains(p.begin(), end, c);
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
        return any_of(cp, [&searchStr](OpenSim::Component const* c)
        {
            return ContainsCaseInsensitive(c->getName(), searchStr);
        });
    }
}

class osc::NavigatorPanel::Impl final : public StandardPanelImpl {
public:
    Impl(
        std::string_view panelName,
        std::shared_ptr<IModelStatePair> model,
        std::function<void(OpenSim::ComponentPath const&)> onRightClick) :

        StandardPanelImpl{panelName},
        m_Model{std::move(model)},
        m_OnRightClick{std::move(onRightClick)}
    {
    }

    bool isOpen() const
    {
        return static_cast<StandardPanelImpl const&>(*this).isOpen();
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

    void implDrawContent() final
    {
        if (!m_Model)
        {
            ui::TextDisabled("(no model)");
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

        ui::Dummy({0.0f, 3.0f});

        // draw filter stuff

        ui::TextUnformatted(ICON_FA_EYE);
        if (ui::BeginPopupContextItem("##filterpopup"))
        {
            ui::Checkbox("frames", &m_ShowFrames);
            ui::EndPopup();
        }
        ui::SameLine();
        DrawSearchBar(m_CurrentSearch);

        ui::Dummy({0.0f, 3.0f});
        ui::Separator();
        ui::Dummy({0.0f, 3.0f});

        // draw content
        ui::BeginChild("##componentnavigatorvieweritems", {0.0, 0.0}, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);

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

        float const unindentPerLevel = ui::GetTreeNodeToLabelSpacing() - 15.0f;

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

                if (!m_ShowFrames && dynamic_cast<OpenSim::FrameGeometry const*>(&c))
                {
                    shouldRender = false;
                }
                else if (auto const* wos = dynamic_cast<OpenSim::WrapObjectSet const*>(&c))
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

            bool const searchHit = hasSearch && isSearchHit(m_CurrentSearch, currentPath);

            // skip rendering if a parent node is collapsed
            if (imguiTreeDepth < currentPath.sizei() - 1)
            {
                continue;
            }

            // pop tree nodes down to the current depth
            while (imguiTreeDepth >= currentPath.sizei())
            {
                ui::Indent(unindentPerLevel);
                ui::TreePop();
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
                ui::PushStyleColor(ImGuiCol_Text, Color::yellow());
                ++styles;
            }
            else if (cur == hover)
            {
                ui::PushStyleColor(ImGuiCol_Text, Color::yellow());
                ++styles;
            }
            else if (!hasSearch || searchHit)
            {
                // display as normal
            }
            else
            {
                ui::PushStyleColor(ImGuiCol_Text, Color::half_grey());
                ++styles;
            }

            // auto-open in these cases
            if (searchHit || currentPath.sizei() == 1 || pathContains(selectionPath, cur))
            {
                ui::SetNextItemOpen(true);
            }

            ui::PushID(imguiId);
            if (ui::TreeNodeEx(cur->getName(), nodeFlags))
            {
                ui::Unindent(unindentPerLevel);
                ++imguiTreeDepth;
            }
            ui::PopID();
            ui::PopStyleColor(styles);

            if (ui::IsItemHovered())
            {
                rv.type = ResponseType::HoverChanged;
                rv.ptr = cur;

                ui::DrawTooltip(cur->getConcreteClassName());
            }

            if (ui::IsItemClicked(ImGuiMouseButton_Left))
            {
                rv.type = ResponseType::SelectionChanged;
                rv.ptr = cur;
            }

            if (ui::IsItemClicked(ImGuiMouseButton_Right))
            {
                m_OnRightClick(GetAbsolutePath(*cur));
            }
        }

        // pop remaining dangling tree elements
        while (imguiTreeDepth-- > 0)
        {
            ui::Indent(unindentPerLevel);
            ui::TreePop();
        }

        ui::EndChild();

        return rv;
    }

    std::shared_ptr<IModelStatePair> m_Model;
    std::function<void(OpenSim::ComponentPath const&)> m_OnRightClick;
    std::string m_CurrentSearch;
    bool m_ShowFrames = false;
};


// public API (PIMPL)

osc::NavigatorPanel::NavigatorPanel(
    std::string_view panelName,
    std::shared_ptr<IModelStatePair> model,
    std::function<void(OpenSim::ComponentPath const&)> onRightClick) :

    m_Impl{std::make_unique<Impl>(panelName, std::move(model), std::move(onRightClick))}
{
}

osc::NavigatorPanel::NavigatorPanel(NavigatorPanel&&) noexcept = default;
osc::NavigatorPanel& osc::NavigatorPanel::operator=(NavigatorPanel&&) noexcept = default;
osc::NavigatorPanel::~NavigatorPanel() noexcept = default;

CStringView osc::NavigatorPanel::implGetName() const
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

void osc::NavigatorPanel::implOnDraw()
{
    m_Impl->onDraw();
}
