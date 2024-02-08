#include "PropertiesPanel.hpp"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.hpp>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.hpp>
#include <OpenSimCreator/UI/ModelEditor/SelectComponentPopup.hpp>
#include <OpenSimCreator/UI/Shared/ObjectPropertiesEditor.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>
#include <oscar/Graphics/Color.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/UI/Panels/StandardPanelImpl.hpp>
#include <oscar/Utils/ScopeGuard.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    void DrawActionsMenu(IEditorAPI* editorAPI, std::shared_ptr<UndoableModelStatePair> const& model)
    {
        OpenSim::Component const* selection = model->getSelected();
        if (!selection)
        {
            return;
        }

        ImGui::Columns(2);
        ImGui::TextUnformatted("actions");
        ImGui::SameLine();
        DrawHelpMarker("Shows a menu containing extra actions that can be performed on this component.\n\nYou can also access the same menu by right-clicking the component in the 3D viewer, bottom status bar, or navigator panel.");
        ImGui::NextColumn();
        PushStyleColor(ImGuiCol_Text, Color::yellow());
        if (ImGui::Button(ICON_FA_BOLT) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            editorAPI->pushComponentContextMenuPopup(GetAbsolutePath(*selection));
        }
        PopStyleColor();
        ImGui::NextColumn();
        ImGui::Columns();
    }

    class ObjectNameEditor final {
    public:
        explicit ObjectNameEditor(std::shared_ptr<UndoableModelStatePair> model_) :
            m_Model{std::move(model_)}
        {
        }

        void onDraw()
        {
            OpenSim::Component const* const selected = m_Model->getSelected();

            if (!selected)
            {
                return;  // don't do anything if nothing is selected
            }

            // update cached edits if model/selection changes
            if (m_Model->getModelVersion() != m_LastModelVersion || selected != m_LastSelected)
            {
                m_EditedName = selected->getName();
                m_LastModelVersion = m_Model->getModelVersion();
                m_LastSelected = selected;
            }

            ImGui::Columns(2);

            ImGui::Separator();
            ImGui::TextUnformatted("name");
            ImGui::SameLine();
            DrawHelpMarker("The name of the component", "The component's name can be important. It can be used when components want to refer to eachover. E.g. a joint will name the two frames it attaches to.");

            ImGui::NextColumn();

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            InputString("##nameeditor", m_EditedName);
            if (ItemValueShouldBeSaved())
            {
                ActionSetComponentName(*m_Model, GetAbsolutePath(*selected), m_EditedName);
            }

            ImGui::NextColumn();

            ImGui::Columns();
        }
    private:
        std::shared_ptr<UndoableModelStatePair> m_Model;
        UID m_LastModelVersion;
        OpenSim::Component const* m_LastSelected = nullptr;
        std::string m_EditedName;
    };
}

class osc::PropertiesPanel::Impl final : public StandardPanelImpl {
public:
    Impl(
        std::string_view panelName,
        IEditorAPI* editorAPI,
        std::shared_ptr<UndoableModelStatePair> model) :

        StandardPanelImpl{panelName},
        m_EditorAPI{editorAPI},
        m_Model{std::move(model)},
        m_SelectionPropertiesEditor{editorAPI, m_Model, [model = m_Model](){ return model->getSelected(); }}
    {
    }

private:
    void implDrawContent() final
    {
        if (!m_Model->getSelected())
        {
            ImGui::TextUnformatted("(nothing selected)");
            return;
        }

        ImGui::PushID(m_Model->getSelected());
        ScopeGuard const g{[]() { ImGui::PopID(); }};

        // draw an actions row with a button that opens the context menu
        //
        // it's helpful to reveal to users that actions are available (#426)
        DrawActionsMenu(m_EditorAPI, m_Model);

        m_NameEditor.onDraw();

        if (!m_Model->getSelected())
        {
            return;
        }

        // property editors
        {
            auto maybeUpdater = m_SelectionPropertiesEditor.onDraw();
            if (maybeUpdater)
            {
                ActionApplyPropertyEdit(*m_Model, *maybeUpdater);
            }
        }
    }

    IEditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;
    ObjectNameEditor m_NameEditor{m_Model};
    ObjectPropertiesEditor m_SelectionPropertiesEditor;
};


// public API (PIMPL)

osc::PropertiesPanel::PropertiesPanel(
    std::string_view panelName,
    IEditorAPI* editorAPI,
    std::shared_ptr<UndoableModelStatePair> model) :
    m_Impl{std::make_unique<Impl>(panelName, editorAPI, std::move(model))}
{
}

osc::PropertiesPanel::PropertiesPanel(PropertiesPanel&&) noexcept = default;
osc::PropertiesPanel& osc::PropertiesPanel::operator=(PropertiesPanel&&) noexcept = default;
osc::PropertiesPanel::~PropertiesPanel() noexcept = default;

CStringView osc::PropertiesPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::PropertiesPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::PropertiesPanel::implOpen()
{
    m_Impl->open();
}

void osc::PropertiesPanel::implClose()
{
    m_Impl->close();
}

void osc::PropertiesPanel::implOnDraw()
{
    m_Impl->onDraw();
}
