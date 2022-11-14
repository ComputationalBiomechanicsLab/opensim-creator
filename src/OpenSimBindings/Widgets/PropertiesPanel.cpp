#include "PropertiesPanel.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/EditorAPI.hpp"
#include "src/OpenSimBindings/Widgets/ComponentContextMenu.hpp"
#include "src/OpenSimBindings/Widgets/ObjectPropertiesEditor.hpp"
#include "src/OpenSimBindings/Widgets/ReassignSocketPopup.hpp"
#include "src/OpenSimBindings/Widgets/SelectComponentPopup.hpp"
#include "src/OpenSimBindings/Widgets/SelectGeometryPopup.hpp"
#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Utils/ScopeGuard.hpp"

#include <IconsFontAwesome5.h>
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


static void DrawActionsMenu(osc::EditorAPI* editorAPI, std::shared_ptr<osc::UndoableModelStatePair> const& model)
{
    OpenSim::Component const* selection = model->getSelected();
    if (!selection)
    {
        return;
    }

    ImGui::Columns(2);
    ImGui::TextUnformatted("actions");
    ImGui::SameLine();
    osc::DrawHelpMarker("Shows a menu containing extra actions that can be performed on this component.\n\nYou can also access the same menu by right-clicking the component in the 3D viewer, bottom status bar, or navigator panel.");
    ImGui::NextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 1.0f, 0.0f, 1.0f});
    if (ImGui::Button(ICON_FA_BOLT))
    {
        editorAPI->pushComponentContextMenuPopup(selection->getAbsolutePath());
    }
    ImGui::PopStyleColor();
    ImGui::NextColumn();
    ImGui::Columns();
}

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

    ImGui::Separator();
    ImGui::TextUnformatted("name");
    ImGui::SameLine();
    osc::DrawHelpMarker("The name of the component", "The component's name can be important. It can be used when components want to refer to eachover. E.g. a joint will name the two frames it attaches to.");

    ImGui::NextColumn();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    std::string nameBuf = selection->getName();  // allowed, because EnterReturnsTrue needs to internally buffer stuff anyway
    if (osc::InputString("##nameeditor", nameBuf, 128, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        osc::ActionSetComponentName(uim, selection->getAbsolutePath(), nameBuf);
    }

    ImGui::NextColumn();

    ImGui::Columns();
    ImGui::PopID();
}

class osc::PropertiesPanel::Impl final {
public:
    Impl(EditorAPI* editorAPI, std::shared_ptr<UndoableModelStatePair> model) :
        m_EditorAPI{std::move(editorAPI)},
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

        // draw an actions row with a button that opens the context menu
        //
        // it's helpful to reveal to users that actions are available (#426)
        DrawActionsMenu(m_EditorAPI, m_Model);

        // draw top-level (not property) editors (e.g. name editor)
        DrawTopLevelMembersEditor(*m_Model);

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
    }

private:
    EditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;
    ObjectPropertiesEditor m_ObjectPropsEditor;
};


// public API (PIMPL)

osc::PropertiesPanel::PropertiesPanel(EditorAPI* editorAPI, std::shared_ptr<UndoableModelStatePair> model) :
    m_Impl{new Impl{std::move(editorAPI), std::move(model)}}
{
}

osc::PropertiesPanel::PropertiesPanel(PropertiesPanel&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::PropertiesPanel& osc::PropertiesPanel::operator=(PropertiesPanel&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::PropertiesPanel::~PropertiesPanel() noexcept
{
    delete m_Impl;
}

void osc::PropertiesPanel::draw()
{
    m_Impl->draw();
}
