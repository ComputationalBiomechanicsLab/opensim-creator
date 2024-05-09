#include "ReassignSocketPopup.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/StringHelpers.h>

#include <algorithm>
#include <compare>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    // parameters that affect which sockets are displayed
    struct PopupParams final {

        PopupParams(
            UID modelVersion_,
            OpenSim::ComponentPath path_,
            std::string socketName_) :

            modelVersion{modelVersion_},
            componentPath{std::move(path_)},
            socketName{std::move(socketName_)}
        {
        }

        friend bool operator==(PopupParams const&, PopupParams const&) = default;

        UID modelVersion;
        OpenSim::ComponentPath componentPath;
        std::string socketName;
        std::string search;
    };

    // a single user-selectable connectee option
    struct ConnecteeOption final {

        explicit ConnecteeOption(OpenSim::Component const& c) :
            absPath{GetAbsolutePath(c)},
            name{c.getName()}
        {
        }

        friend auto operator<=>(ConnecteeOption const& lhs, ConnecteeOption const& rhs)
        {
            return std::tie(lhs.name, lhs.absPath.toString()) <=> std::tie(rhs.name, rhs.absPath.toString());
        }

        [[maybe_unused]]  // TODO: Ubuntu20 doesn't use this function
        friend bool operator==(ConnecteeOption const& lhs, ConnecteeOption const& rhs)
        {
            return std::tie(lhs.name, lhs.absPath.toString()) == std::tie(rhs.name, rhs.absPath.toString());
        }

        OpenSim::ComponentPath absPath;
        std::string name;
    };

    // generate a list of possible connectee options, given a set of popup parameters
    std::vector<ConnecteeOption> GenerateSelectionOptions(
        OpenSim::Model const& model,
        PopupParams const& params)
    {
        std::vector<ConnecteeOption> rv;

        OpenSim::Component const* component = FindComponent(model, params.componentPath);
        if (!component)
        {
            return rv;   // component isn't in model?
        }

        OpenSim::AbstractSocket const* socket = FindSocket(*component, params.socketName);
        if (!socket)
        {
            return rv;  // socket isn't in model?
        }

        for (OpenSim::Component const& other : model.getComponentList())
        {
            if (&other == component)
            {
                continue;  // hide redundant reconnnections
            }

            if (!contains(other.getName(), params.search))
            {
                continue;  // filtered out by search string
            }

            if (!IsAbleToConnectTo(*socket, other))
            {
                continue;  // connection would be rejected anyway
            }

            rv.emplace_back(other);
        }

        rgs::sort(rv);

        return rv;
    }
}

class osc::ReassignSocketPopup::Impl final : public StandardPopup {
public:
    Impl(std::string_view popupName,
         std::shared_ptr<UndoableModelStatePair> model,
         std::string_view componentAbsPath,
         std::string_view socketName) :

        StandardPopup{popupName},
        m_Model{std::move(model)},
        m_Params{m_Model->getModelVersion(), std::string{componentAbsPath}, std::string{socketName}}
    {
    }

private:
    void impl_draw_content() final
    {
        // caching: regenerate cached socket list, if necessary
        //
        // we cache the list because searching+filtering all possible connectees is
        // very slow in OpenSim (#384)
        m_EditedParams.modelVersion = m_Model->getModelVersion();
        if (m_EditedParams != m_Params)
        {
            m_Options = GenerateSelectionOptions(m_Model->getModel(), m_EditedParams);
            m_Params = m_EditedParams;
        }

        // check: ensure the "from" side of the socket still exists
        OpenSim::Component const* component = FindComponent(m_Model->getModel(), m_Params.componentPath);
        if (!component)
        {
            request_close();
            return;
        }

        // check: ensure the socket still exists
        OpenSim::AbstractSocket const* socket = FindSocket(*component, m_Params.socketName);
        if (!socket)
        {
            request_close();
            return;
        }

        // draw UI

        ui::draw_text("connect %s (%s) to:", socket->getName().c_str(), socket->getConnecteeTypeName().c_str());

        ui::draw_dummy({0.0f, 0.1f * ui::get_text_line_height()});
        ui::draw_separator();
        ui::draw_dummy({0.0f, 0.25f * ui::get_text_line_height()});

        DrawSearchBar(m_EditedParams.search);

        std::optional<OpenSim::ComponentPath> userSelection;
        ui::begin_child_panel("##componentlist", Vec2{512.0f, 256.0f}, ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

        int id = 0;  // care: necessary because multiple connectees may have the same name
        for (ConnecteeOption const& option : m_Options)
        {
            ui::push_id(id++);
            if (ui::draw_selectable(option.name))
            {
                userSelection = option.absPath;
            }
            ui::draw_tooltip_if_item_hovered(option.absPath.toString());
            ui::pop_id();
        }
        ui::end_child_panel();

        if (!m_Error.empty())
        {
            ui::set_next_item_width(ui::get_content_region_avail().x);
            ui::draw_text_wrapped(m_Error);
        }

        // add ability to re-express a component in a new frame (#326)
        tryDrawReexpressPropertyInFrameCheckbox(*component, *socket);

        if (ui::draw_button("Cancel"))
        {
            request_close();
            return;
        }

        // if the user selected something, try to form the connection in the active model
        if (userSelection)
        {
            SocketReassignmentFlags const flags = m_TryReexpressInDifferentFrame ?
                SocketReassignmentFlags::TryReexpressComponentInNewConnectee :
                SocketReassignmentFlags::None;

            OpenSim::Component const* selected = FindComponent(m_Model->getModel(), *userSelection);

            if (selected && ActionReassignComponentSocket(*m_Model, m_Params.componentPath, m_Params.socketName, *selected, flags, m_Error))
            {
                request_close();
                return;
            }
        }
    }

    void impl_on_close() final
    {
        m_EditedParams.search.clear();
        m_Error.clear();
    }

    void tryDrawReexpressPropertyInFrameCheckbox(
        OpenSim::Component const& component,
        OpenSim::AbstractSocket const& abstractSocket)
    {
        std::string const label = [&component]()
        {
            std::stringstream ss;
            ss << "Re-express " << component.getName() << " in chosen frame";
            return std::move(ss).str();
        }();

        auto const* const physFrameSocket =
            dynamic_cast<OpenSim::Socket<OpenSim::PhysicalFrame> const*>(&abstractSocket);
        if (!physFrameSocket)
        {
            bool v = false;
            ui::draw_checkbox(label, &v);
            ui::draw_tooltip_body_only_if_item_hovered("Disabled: the socket doesn't connect to a physical frame");
            return;
        }

        auto const componentSpatialRepresentation =
            TryGetSpatialRepresentation(component, m_Model->getState());
        if (!componentSpatialRepresentation)
        {
            bool v = false;
            ui::draw_checkbox(label, &v);
            ui::draw_tooltip_body_only_if_item_hovered("Disabled: the component doesn't have a spatial representation that OSC knows how to re-express");
            return;
        }

        ui::draw_checkbox(label, &m_TryReexpressInDifferentFrame);
    }

    std::shared_ptr<UndoableModelStatePair> m_Model;
    PopupParams m_Params;
    PopupParams m_EditedParams = m_Params;
    std::vector<ConnecteeOption> m_Options = GenerateSelectionOptions(m_Model->getModel(), m_EditedParams);
    std::string m_Error;
    bool m_TryReexpressInDifferentFrame = false;
};


// public API (PIMPL)

osc::ReassignSocketPopup::ReassignSocketPopup(
    std::string_view popupName,
    std::shared_ptr<UndoableModelStatePair> model,
    std::string_view componentAbsPath,
    std::string_view socketName) :

    m_Impl{std::make_unique<Impl>(popupName, std::move(model), componentAbsPath, socketName)}
{
}

osc::ReassignSocketPopup::ReassignSocketPopup(ReassignSocketPopup&&) noexcept = default;
osc::ReassignSocketPopup& osc::ReassignSocketPopup::operator=(ReassignSocketPopup&&) noexcept = default;
osc::ReassignSocketPopup::~ReassignSocketPopup() noexcept = default;

bool osc::ReassignSocketPopup::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::ReassignSocketPopup::impl_open()
{
    m_Impl->open();
}

void osc::ReassignSocketPopup::impl_close()
{
    m_Impl->close();
}

bool osc::ReassignSocketPopup::impl_begin_popup()
{
    return m_Impl->begin_popup();
}

void osc::ReassignSocketPopup::impl_on_draw()
{
    m_Impl->on_draw();
}

void osc::ReassignSocketPopup::impl_end_popup()
{
    m_Impl->end_popup();
}
