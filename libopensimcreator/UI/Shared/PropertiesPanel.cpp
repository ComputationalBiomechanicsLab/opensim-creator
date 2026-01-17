#include "PropertiesPanel.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Platform/msmicons.h>
#include <libopensimcreator/UI/Events/OpenComponentContextMenuEvent.h>
#include <libopensimcreator/UI/Shared/ObjectPropertiesEditor.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/graphics/color.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/widget.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/utils/scope_exit.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    void DrawActionsMenu(Widget& parent, const std::shared_ptr<IModelStatePair>& model)
    {
        const OpenSim::Component* const selection = model->getSelected();
        if (not selection) {
            return;
        }

        ui::set_num_columns(2);
        ui::align_text_to_frame_padding();  // ensure it aligns with the button in the next column
        ui::draw_text("actions");
        ui::same_line();
        ui::draw_help_marker("Shows a menu containing extra actions that can be performed on this component.\n\nYou can also access the same menu by right-clicking the component in the 3D viewer, bottom status bar, or navigator panel.");
        ui::next_column();
        ui::push_style_color(ui::ColorVar::Text, Color::yellow());
        if (ui::draw_button(MSMICONS_BOLT) or ui::is_item_clicked(ui::MouseButton::Right)) {
            App::post_event<OpenComponentContextMenuEvent>(parent, GetAbsolutePath(*selection));
        }
        ui::pop_style_color();
        ui::next_column();
        ui::set_num_columns();
    }

    class ObjectNameEditor final {
    public:
        explicit ObjectNameEditor(std::shared_ptr<IModelStatePair> model_) :
            m_Model{std::move(model_)}
        {}

        void onDraw()
        {
            const OpenSim::Component* const selected = m_Model->getSelected();
            if (not selected) {
                return;  // don't do anything if nothing is selected
            }

            // update cached edits if model/selection changes
            if (m_Model->getModelVersion() != m_LastModelVersion or selected != m_LastSelected) {
                m_EditedName = selected->getName();
                m_LastModelVersion = m_Model->getModelVersion();
                m_LastSelected = selected;
            }

            const bool disabled = m_Model->isReadonly();
            if (disabled) {
                ui::begin_disabled();
            }

            ui::set_num_columns(2);

            ui::draw_separator();
            ui::align_text_to_frame_padding();  // ensure it aligns with the next column
            ui::draw_text("name");
            ui::same_line();
            ui::draw_help_marker("The name of the component", "The component's name can be important. It can be used when components want to refer to eachover. E.g. a joint will name the two frames it attaches to.");

            ui::next_column();

            ui::set_next_item_width(ui::get_content_region_available().x());
            ui::draw_string_input("##nameeditor", m_EditedName);
            if (ui::should_save_last_drawn_item_value()) {
                ActionSetComponentName(*m_Model, GetAbsolutePath(*selected), m_EditedName);
            }
            ui::add_screenshot_annotation_to_last_drawn_item("PropertiesPanel/name");

            ui::next_column();

            ui::set_num_columns();

            if (disabled) {
                ui::end_disabled();
            }
        }
    private:
        std::shared_ptr<IModelStatePair> m_Model;
        UID m_LastModelVersion;
        const OpenSim::Component* m_LastSelected = nullptr;
        std::string m_EditedName;
    };
}

class osc::PropertiesPanel::Impl final : public PanelPrivate {
public:
    explicit Impl(
        PropertiesPanel& owner,
        Widget* parent,
        std::string_view panelName,
        std::shared_ptr<IModelStatePair> model) :

        PanelPrivate{owner, parent, panelName},
        m_Model{std::move(model)},
        m_SelectionPropertiesEditor{&owner, m_Model, [model = m_Model](){ return model->getSelected(); }}
    {}

    void draw_content()
    {
        if (not m_Model->getSelected()) {
            ui::draw_text_disabled_and_panel_centered("(nothing selected)");
            return;
        }

        ui::push_id(m_Model->getSelected());
        const ScopeExit g{[]{ ui::pop_id(); }};

        // draw an actions row with a button that opens the context menu
        //
        // it's helpful to reveal to users that actions are available (#426)
        DrawActionsMenu(*parent(), m_Model);

        m_NameEditor.onDraw();

        if (not m_Model->getSelected()) {
            return;
        }

        // property editors
        {
            auto maybeUpdater = m_SelectionPropertiesEditor.onDraw();
            if (maybeUpdater) {
                ActionApplyPropertyEdit(*m_Model, *maybeUpdater);
            }
        }
    }

private:
    std::shared_ptr<IModelStatePair> m_Model;
    ObjectNameEditor m_NameEditor{m_Model};
    ObjectPropertiesEditor m_SelectionPropertiesEditor;
};

osc::PropertiesPanel::PropertiesPanel(
    Widget* parent,
    std::string_view panelName,
    std::shared_ptr<IModelStatePair> model) :
    Panel{std::make_unique<Impl>(*this, parent, panelName, std::move(model))}
{}
void osc::PropertiesPanel::impl_draw_content() { private_data().draw_content(); }
