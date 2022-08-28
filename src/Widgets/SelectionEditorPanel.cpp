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
    }

private:
    std::shared_ptr<UndoableModelStatePair> m_Model;
    ObjectPropertiesEditor m_ObjectPropsEditor;
};


// public API (PIMPL)

osc::SelectionEditorPanel::SelectionEditorPanel(std::shared_ptr<UndoableModelStatePair> model) :
    m_Impl{new Impl{std::move(model)}}
{
}

osc::SelectionEditorPanel::SelectionEditorPanel(SelectionEditorPanel&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SelectionEditorPanel& osc::SelectionEditorPanel::operator=(SelectionEditorPanel&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::SelectionEditorPanel::~SelectionEditorPanel() noexcept
{
    delete m_Impl;
}

void osc::SelectionEditorPanel::draw()
{
    m_Impl->draw();
}
