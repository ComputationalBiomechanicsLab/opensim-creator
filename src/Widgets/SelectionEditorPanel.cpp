#include "SelectionEditorPanel.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Widgets/ObjectPropertiesEditor.hpp"
#include "src/Widgets/ReassignSocketPopup.hpp"
#include "src/Widgets/SelectComponentPopup.hpp"
#include "src/Widgets/SelectGeometryPopup.hpp"

#include <imgui.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathActuator.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// draw an editor for top-level selected Component members (e.g. name)
static void DrawTopLevelMembersEditor(osc::UndoableModelStatePair& uim)
{
    OpenSim::Component const* selection = uim.getSelected();

    if (!selection)
    {
        ImGui::Text("cannot draw top level editor: nothing selected?");
        return;
    }

    ImGui::PushID(selection);
    ImGui::Columns(2);

    ImGui::TextUnformatted("name");
    ImGui::SameLine();
    osc::DrawHelpMarker("The name of the component", "The component's name can be important. It can be used when components want to refer to eachover. E.g. a joint will name the two frames it attaches to.");

    ImGui::NextColumn();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    std::string nameBuf = selection->getName();  // allowed, because EnterReturnsTrue needs to internally buffer stuff anyway
    if (osc::InputString("##nameeditor", nameBuf, 128, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        osc::ActionSetComponentName(uim, selection->getAbsolutePath(), nameBuf);
    }

    ImGui::NextColumn();

    ImGui::Columns();
    ImGui::PopID();
}

// draw socket editor for current selection
static void DrawSocketEditor(std::optional<osc::ReassignSocketPopup>& reassignPopup, std::shared_ptr<osc::UndoableModelStatePair> const& uim)
{
    OpenSim::Component const* selected = uim->getSelected();

    if (!selected)
    {
        ImGui::TextUnformatted("cannot draw socket editor: selection is blank (shouldn't be)");
        return;
    }

    std::vector<std::string> socknames = osc::GetSocketNames(*selected);

    if (socknames.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, OSC_GREYED_RGBA);
        ImGui::Text("    (OpenSim::%s has no sockets)", selected->getConcreteClassName().c_str());
        ImGui::PopStyleColor();
        return;
    }

    // else: it has sockets with names, list each socket and provide the user
    //       with the ability to reassign the socket's connectee

    ImGui::Columns(2);
    for (std::string const& sn : socknames)
    {
        ImGui::TextUnformatted(sn.c_str());
        ImGui::NextColumn();

        OpenSim::AbstractSocket const& socket = selected->getSocket(sn);
        std::string sockname = socket.getConnecteePath();
        std::string popupname = std::string{"reassign "} + sockname;

        if (ImGui::Button(sockname.c_str()))
        {
            reassignPopup.emplace(popupname, uim, selected->getAbsolutePathString(), socket.getName());
            reassignPopup->open();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(socket.getConnecteeAsObject().getName().c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("%s", socket.getConnecteeAsObject().getConcreteClassName().c_str());
            ImGui::NewLine();
            ImGui::TextDisabled("Left-Click: Reassign this socket's connectee");
            ImGui::TextDisabled("Right-Click: Select the connectee");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject()))
        {
            uim->setSelected(dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject()));
            ImGui::NextColumn();
            break;  // don't continue to traverse the sockets, because the selection changed
        }

        ImGui::NextColumn();
    }
    ImGui::Columns();

    if (reassignPopup)
    {
        reassignPopup->draw();
    }
}

class osc::SelectionEditorPanel::Impl final {
public:
    Impl(std::shared_ptr<UndoableModelStatePair> model) :
        m_Model{std::move(model)}
    {
    }

    void draw()
    {
        if (!m_Model->getSelected())
        {
            ImGui::TextUnformatted("(nothing selected)");
            return;
        }

        ImGui::PushID(m_Model->getSelected());
        OSC_SCOPE_GUARD({ ImGui::PopID(); });

        // top-level property editors
        {
            DrawTopLevelMembersEditor(*m_Model);
        }

        if (!m_Model->getSelected())
        {
            return;
        }

        // property editors
        {
            auto maybeUpdater = m_ObjectPropsEditor.draw(*m_Model->getSelected());
            if (maybeUpdater)
            {
                osc::ActionApplyPropertyEdit(*m_Model, *maybeUpdater);
            }
        }

        if (!m_Model->getSelected())
        {
            return;
        }

        // socket editor
        ImGui::Dummy({0.0f, 5.0f});
        ImGui::TextUnformatted("sockets:");
        ImGui::SameLine();
        osc::DrawHelpMarker("What components this component is connected to.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ImGui::Separator();
        DrawSocketEditor(m_MaybeReassignSocketPopup, m_Model);
    }

private:
    std::shared_ptr<UndoableModelStatePair> m_Model;
    std::optional<ReassignSocketPopup> m_MaybeReassignSocketPopup;
    ObjectPropertiesEditor m_ObjectPropsEditor;
};

osc::SelectionEditorPanel::SelectionEditorPanel(std::shared_ptr<UndoableModelStatePair> model) :
    m_Impl{new Impl{std::move(model)}}
{
}
osc::SelectionEditorPanel::SelectionEditorPanel(SelectionEditorPanel&&) noexcept = default;
osc::SelectionEditorPanel& osc::SelectionEditorPanel::operator=(SelectionEditorPanel&&) noexcept = default;
osc::SelectionEditorPanel::~SelectionEditorPanel() noexcept = default;

void osc::SelectionEditorPanel::draw()
{
    m_Impl->draw();
}
