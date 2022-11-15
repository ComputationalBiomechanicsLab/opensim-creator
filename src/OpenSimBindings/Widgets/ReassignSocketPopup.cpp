#include "ReassignSocketPopup.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Widgets/StandardPopup.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    // parameters that affect which sockets are displayed
    struct PopupParams final {
        osc::UID modelVersion;
        OpenSim::ComponentPath path;
        std::string socketName;
        std::string search;

        PopupParams(
            osc::UID modelVersion_,
            OpenSim::ComponentPath path_,
            std::string socketName_) :

            modelVersion{std::move(modelVersion_)},
            path{std::move(path_)},
            socketName{std::move(socketName_)}
        {
        }
    };

    bool operator==(PopupParams const& a, PopupParams const& b)
    {
        return
            a.modelVersion == b.modelVersion &&
            a.path == b.path &&
            a.socketName == b.socketName &&
            a.search == b.search;
    }

    bool operator!=(PopupParams const& a, PopupParams const& b)
    {
        return !(a == b);
    }

    // a single user-selectable connectee option
    struct ConnecteeOption final {
        OpenSim::ComponentPath absPath;
        std::string name;

        explicit ConnecteeOption(OpenSim::Component const& c) :
            absPath{c.getAbsolutePath()},
            name{c.getName()}
        {
        }
    };

    // generate a list of possible connectee options, given a set of popup parameters
    std::vector<ConnecteeOption> GenerateSelectionOptions(OpenSim::Model const& model, PopupParams const& params)
    {
        std::vector<ConnecteeOption> rv;

        OpenSim::Component const* component = osc::FindComponent(model, params.path);
        if (!component)
        {
            return rv;   // component isn't in model?
        }

        OpenSim::AbstractSocket const* socket = osc::FindSocket(*component, params.socketName);
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

            if (!osc::ContainsSubstring(other.getName(), params.search))
            {
                continue;  // filtered out by search string
            }

            if (!osc::IsAbleToConnectTo(*socket, other))
            {
                continue;  // connection would be rejected anyway
            }

            rv.emplace_back(other);
        }

        return rv;
    }
}

class osc::ReassignSocketPopup::Impl final : public osc::StandardPopup {
public:
    Impl(std::string_view popupName,
         std::shared_ptr<UndoableModelStatePair> model,
         std::string_view componentAbsPath,
         std::string_view socketName) :

        StandardPopup{std::move(popupName)},
        m_Model{std::move(model)},
        m_Params{m_Model->getModelVersion(), std::string{componentAbsPath}, std::string{socketName}}
    {
    }

private:
    void implDrawContent() override
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
        OpenSim::Component const* component = osc::FindComponent(m_Model->getModel(), m_Params.path);
        if (!component)
        {
            requestClose();
            return;
        }

        // check: ensure the socket still exists
        OpenSim::AbstractSocket const* socket = osc::FindSocket(*component, m_Params.socketName);
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

        osc::DrawSearchBar(m_EditedParams.search, 256);

        std::optional<OpenSim::ComponentPath> userSelection;
        int id = 0;  // care: necessary because multiple connectees may have the same name
        ImGui::BeginChild("##componentlist", ImVec2(512, 256), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
        for (ConnecteeOption const& option : m_Options)
        {
            ImGui::PushID(id++);
            if (ImGui::Selectable(option.name.c_str()))
            {
                userSelection = option.absPath;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        if (!m_Error.empty())
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::TextWrapped("%s", m_Error.c_str());
        }

        if (ImGui::Button("Cancel"))
        {
            requestClose();
            return;
        }

        // if the user selected something, try to form the connection in the active model
        if (userSelection)
        {
            OpenSim::Component const* selected = osc::FindComponent(m_Model->getModel(), *userSelection);
            if (selected && osc::ActionReassignComponentSocket(*m_Model, m_Params.path, m_Params.socketName, *selected, m_Error))
            {
                requestClose();
                return;
            }
        }
    }

    void implOnClose() override
    {
        m_EditedParams.search.clear();
        m_Error.clear();
    }

    std::shared_ptr<UndoableModelStatePair> m_Model;
    PopupParams m_Params;
    PopupParams m_EditedParams = m_Params;
    std::vector<ConnecteeOption> m_Options = GenerateSelectionOptions(m_Model->getModel(), m_EditedParams);
    std::string m_Error;
};


// public API (PIMPL)

osc::ReassignSocketPopup::ReassignSocketPopup(
    std::string_view popupName,
    std::shared_ptr<UndoableModelStatePair> model,
    std::string_view componentAbsPath,
    std::string_view socketName) :

    m_Impl{std::make_unique<Impl>(std::move(popupName), std::move(model), std::move(componentAbsPath), std::move(socketName))}
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

void osc::ReassignSocketPopup::implDrawPopupContent()
{
    m_Impl->drawPopupContent();
}

void osc::ReassignSocketPopup::implEndPopup()
{
    m_Impl->endPopup();
}