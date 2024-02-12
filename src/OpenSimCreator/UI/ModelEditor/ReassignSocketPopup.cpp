#include "ReassignSocketPopup.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/StringHelpers.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

using namespace osc;

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

        OpenSim::ComponentPath absPath;
        std::string name;
    };

    bool operator<(ConnecteeOption const& lhs, ConnecteeOption const& rhs)
    {
        return std::tie(lhs.name, lhs.absPath.toString()) < std::tie(rhs.name, rhs.absPath.toString());
    }

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

            if (!Contains(other.getName(), params.search))
            {
                continue;  // filtered out by search string
            }

            if (!IsAbleToConnectTo(*socket, other))
            {
                continue;  // connection would be rejected anyway
            }

            rv.emplace_back(other);
        }

        std::sort(rv.begin(), rv.end());

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
    void implDrawContent() final
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
            requestClose();
            return;
        }

        // check: ensure the socket still exists
        OpenSim::AbstractSocket const* socket = FindSocket(*component, m_Params.socketName);
        if (!socket)
        {
            requestClose();
            return;
        }

        // draw UI

        ImGui::Text("connect %s (%s) to:", socket->getName().c_str(), socket->getConnecteeTypeName().c_str());

        ImGui::Dummy({0.0f, 0.1f * ImGui::GetTextLineHeight()});
        ImGui::Separator();
        ImGui::Dummy({0.0f, 0.25f * ImGui::GetTextLineHeight()});

        DrawSearchBar(m_EditedParams.search);

        std::optional<OpenSim::ComponentPath> userSelection;
        ImGui::BeginChild("##componentlist", ImVec2(512, 256), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

        int id = 0;  // care: necessary because multiple connectees may have the same name
        for (ConnecteeOption const& option : m_Options)
        {
            ImGui::PushID(id++);
            if (ImGui::Selectable(option.name.c_str()))
            {
                userSelection = option.absPath;
            }
            DrawTooltipIfItemHovered(option.absPath.toString());
            ImGui::PopID();
        }
        ImGui::EndChild();

        if (!m_Error.empty())
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::TextWrapped("%s", m_Error.c_str());
        }

        // add ability to re-express a component in a new frame (#326)
        tryDrawReexpressPropertyInFrameCheckbox(*component, *socket);

        if (ImGui::Button("Cancel"))
        {
            requestClose();
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
                requestClose();
                return;
            }
        }
    }

    void implOnClose() final
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
            ImGui::Checkbox(label.c_str(), &v);
            DrawTooltipBodyOnlyIfItemHovered("Disabled: the socket doesn't connect to a physical frame");
            return;
        }

        auto const componentSpatialRepresentation =
            TryGetSpatialRepresentation(component, m_Model->getState());
        if (!componentSpatialRepresentation)
        {
            bool v = false;
            ImGui::Checkbox(label.c_str(), &v);
            DrawTooltipBodyOnlyIfItemHovered("Disabled: the component doesn't have a spatial representation that OSC knows how to re-express");
            return;
        }

        ImGui::Checkbox(label.c_str(), &m_TryReexpressInDifferentFrame);
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

bool osc::ReassignSocketPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::ReassignSocketPopup::implOpen()
{
    m_Impl->open();
}

void osc::ReassignSocketPopup::implClose()
{
    m_Impl->close();
}

bool osc::ReassignSocketPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::ReassignSocketPopup::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::ReassignSocketPopup::implEndPopup()
{
    m_Impl->endPopup();
}
