#include "PropertiesPanel.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/UI/ModelEditor/SelectComponentPopup.h>
#include <OpenSimCreator/UI/Shared/ObjectPropertiesEditor.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>
#include <oscar/Graphics/Color.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/Utils/ScopeGuard.h>

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

        ui::Columns(2);
        ui::TextUnformatted("actions");
        ui::SameLine();
        ui::DrawHelpMarker("Shows a menu containing extra actions that can be performed on this component.\n\nYou can also access the same menu by right-clicking the component in the 3D viewer, bottom status bar, or navigator panel.");
        ui::NextColumn();
        ui::PushStyleColor(ImGuiCol_Text, Color::yellow());
        if (ui::Button(ICON_FA_BOLT) || ui::IsItemClicked(ImGuiMouseButton_Right))
        {
            editorAPI->pushComponentContextMenuPopup(GetAbsolutePath(*selection));
        }
        ui::PopStyleColor();
        ui::NextColumn();
        ui::Columns();
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

            ui::Columns(2);

            ui::Separator();
            ui::TextUnformatted("name");
            ui::SameLine();
            ui::DrawHelpMarker("The name of the component", "The component's name can be important. It can be used when components want to refer to eachover. E.g. a joint will name the two frames it attaches to.");

            ui::NextColumn();

            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            ui::InputString("##nameeditor", m_EditedName);
            if (ui::ItemValueShouldBeSaved())
            {
                ActionSetComponentName(*m_Model, GetAbsolutePath(*selected), m_EditedName);
            }

            ui::NextColumn();

            ui::Columns();
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
    void impl_draw_content() final
    {
        if (!m_Model->getSelected())
        {
            ui::TextDisabledAndWindowCentered("(nothing selected)");
            return;
        }

        ui::PushID(m_Model->getSelected());
        ScopeGuard const g{[]() { ui::PopID(); }};

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

CStringView osc::PropertiesPanel::impl_get_name() const
{
    return m_Impl->name();
}

bool osc::PropertiesPanel::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::PropertiesPanel::impl_open()
{
    m_Impl->open();
}

void osc::PropertiesPanel::impl_close()
{
    m_Impl->close();
}

void osc::PropertiesPanel::impl_on_draw()
{
    m_Impl->on_draw();
}
