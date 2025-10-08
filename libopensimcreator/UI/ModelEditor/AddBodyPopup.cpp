#include "AddBodyPopup.h"

#include <libopensimcreator/ComponentRegistry/ComponentRegistry.h>
#include <libopensimcreator/ComponentRegistry/StaticComponentRegistries.h>
#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Platform/IconCodepoints.h>
#include <libopensimcreator/UI/ModelEditor/SelectGeometryPopup.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Platform/App.h>
#include <liboscar/Platform/Widget.h>
#include <liboscar/UI/Events/OpenPopupEvent.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Popups/Popup.h>
#include <liboscar/UI/Popups/PopupPrivate.h>
#include <liboscar/Utils/StringHelpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Ground.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>


class osc::AddBodyPopup::Impl final : public PopupPrivate {
public:
    Impl(
        AddBodyPopup& owner,
        Widget* parent,
        std::string_view popupName,
        std::shared_ptr<IModelStatePair> modelState) :

        PopupPrivate{owner, parent, popupName},
        m_Model{std::move(modelState)}
    {}

    void draw_content()
    {
        if (m_Model->isReadonly()) {
            ui::draw_text_centered(OSC_ICON_LOCK " cannot edit the model - it is locked");
            if (ui::draw_button("cancel")) {
                request_close();
            }
            return;
        }

        const OpenSim::Model& model = m_Model->getModel();

        const auto* selectedPf = FindComponent<OpenSim::PhysicalFrame>(model, m_BodyDetails.parentFrameAbsPath);
        if (not selectedPf) {
            // if nothing selected (or not found), coerce the initial selection to ground
            selectedPf = &model.getGround();
            m_BodyDetails.parentFrameAbsPath = GetAbsolutePathString(*selectedPf);
        }

        ui::set_num_columns(2);

        // prompt name
        {
            if (is_popup_opened_this_frame()) {
                ui::set_keyboard_focus_here();
            }

            ui::draw_text("body name");
            ui::same_line();
            ui::draw_help_marker("The name used to identify the OpenSim::Body in the model. OpenSim typically uses the name to identify connections between components in a model, so the name should be unique.");
            ui::next_column();
            ui::set_next_item_width(ui::get_content_region_available().x);
            ui::draw_string_input("##bodyname", m_BodyDetails.bodyName);
            ui::add_screenshot_annotation_to_last_drawn_item("AddBodyPopup::BodyNameInput");
            ui::next_column();
        }

        // prompt mass
        {
            ui::draw_text("mass (kg)");
            ui::same_line();
            ui::draw_help_marker("The mass of the body in kilograms");
            ui::next_column();
            ui::set_next_item_width(ui::get_content_region_available().x);
            ui::draw_float_kilogram_input("##mass", m_BodyDetails.mass);
            ui::next_column();
        }

        // prompt center of mass
        {
            ui::draw_text("center of mass");
            ui::same_line();
            ui::draw_help_marker("The location of the mass center in the body frame.");
            ui::next_column();
            ui::set_next_item_width(ui::get_content_region_available().x);
            ui::draw_float3_meters_input("##comeditor", m_BodyDetails.centerOfMass);
            ui::next_column();
        }

        // prompt inertia
        {
            ui::draw_text("inertia (tensor)");
            ui::same_line();
            ui::draw_help_marker("The elements of the inertia tensor (Vec6) as [Ixx Iyy Izz Ixy Ixz Iyz]. These are measured about the center of mass, *not* the center of the body frame.");
            ui::next_column();
            ui::set_next_item_width(ui::get_content_region_available().x);
            ui::draw_float3_meters_input("##inertiaeditor", m_BodyDetails.inertia);
            ui::next_column();
        }

        // prompt body/ground that new body will connect to (via a joint)
        {
            ui::draw_text("join to");
            ui::same_line();
            ui::draw_help_marker("What the added body will be joined to. All bodies in an OpenSim model are connected to other bodies, or the ground, by joints. This is true even if the joint is unconstrained and does nothing (e.g. an OpenSim::FreeJoint) or if the joint constrains motion in all direcctions (e.g. an OpenSim::WeldJoint).");
            ui::next_column();

            // show a search bar that the user can type into in order to filter through
            // the available frame list (can contain many items in large models, #21).
            ui::set_next_item_width(ui::get_content_region_available().x);
            DrawSearchBar(m_JoinToSearchFilter);

            ui::begin_child_panel("join targets", Vector2{0, 128.0f}, ui::ChildPanelFlag::Border, ui::PanelFlag::HorizontalScrollbar);
            for (const auto& pf : model.getComponentList<OpenSim::PhysicalFrame>()) {
                if (not contains_case_insensitive(pf.getName(), m_JoinToSearchFilter)) {
                    continue;
                }
                if (ui::draw_selectable(pf.getName(), &pf == selectedPf)) {
                    selectedPf = &pf;
                    m_BodyDetails.parentFrameAbsPath = GetAbsolutePathString(*selectedPf);
                }
                if (&pf == selectedPf) {
                    ui::add_screenshot_annotation_to_last_drawn_item(pf.getName());
                }
            }
            ui::end_child_panel();
            ui::next_column();
        }

        // prompt joint type for the above
        {
            ui::draw_text("joint type");
            ui::same_line();
            ui::draw_help_marker("The type of OpenSim::Joint that will connect the new OpenSim::Body to the selection above");
            ui::next_column();
            {
                const auto& registry = GetComponentRegistry<OpenSim::Joint>();
                ui::draw_combobox(
                    "##jointtype",
                    &m_BodyDetails.jointTypeIndex,
                    registry.size(),
                    [&registry](size_t i) { return registry[i].name(); }
                );
                ui::add_screenshot_annotation_to_last_drawn_item("AddBodyPopup::JointTypeInput");
            }
            ui::next_column();
        }

        // prompt joint name
        {
            ui::draw_text("joint name");
            ui::same_line();
            ui::draw_help_marker("The name of the OpenSim::Joint that will join the new body to the existing frame specified above");
            ui::next_column();
            ui::set_next_item_width(ui::get_content_region_available().x);
            ui::draw_string_input("##jointnameinput", m_BodyDetails.jointName);
            ui::add_screenshot_annotation_to_last_drawn_item("AddBodyPopup::JointNameInput");
            ui::next_column();
        }

        // prompt adding offset frames
        {
            ui::draw_text("add offset frames");
            ui::same_line();
            ui::draw_help_marker("Whether osc should automatically add intermediate offset frames to the OpenSim::Joint. A joint can attach to the two bodies (this added one, plus the selected one) directly. However, many OpenSim model designs instead make the joint attach to offset frames which, themselves, attach to the bodies. The utility of doing this is that the offset frames can be manually adjusted later, rather than *having* to attach the center of the joint to the center of the body");
            ui::next_column();
            ui::draw_checkbox("##addoffsetframescheckbox", &m_BodyDetails.addOffsetFrames);
            ui::add_screenshot_annotation_to_last_drawn_item("AddBodyPopup::AddOffsetFramesInput");
            ui::next_column();
        }

        // prompt geometry
        {
            ui::draw_text("geometry");
            ui::same_line();
            ui::draw_help_marker("Attaches visual geometry to the new body. This is what the OpenSim::Body looks like in the UI. The geometry is purely cosmetic and does not affect the simulation");
            ui::next_column();
            {
                const std::string label = m_BodyDetails.maybeGeometry ? GetDisplayName(*m_BodyDetails.maybeGeometry) : std::string{"attach"};

                if (ui::draw_button(label)) {
                    // open geometry selection popup
                    auto popup = std::make_unique<SelectGeometryPopup>(
                        &owner(),
                        "addbody_attachgeometry",
                        App::resource_filepath("geometry"),
                        [this](auto ptr) { onGeometrySelection(std::move(ptr)); }
                    );
                    App::post_event<OpenPopupEvent>(owner(), std::move(popup));
                }
                ui::add_screenshot_annotation_to_last_drawn_item("AddBodyPopup::GeometryButton");
            }
            ui::next_column();
        }

        ui::set_num_columns();

        // end of input prompting: show user cancel/ok buttons

        if (ui::draw_button("cancel")) {
            request_close();
        }

        ui::same_line();

        if (ui::draw_button(OSC_ICON_PLUS " add body")) {
            ActionAddBodyToModel(*m_Model, m_BodyDetails);
            request_close();
        }
    }

    void on_close()
    {
        m_BodyDetails = BodyDetails{};
    }
private:
    void onGeometrySelection(std::unique_ptr<OpenSim::Geometry> ptr)
    {
        m_BodyDetails.maybeGeometry = std::move(ptr);
    }

    // the model that the body will be added to
    std::shared_ptr<IModelStatePair> m_Model;

    // a user-enacted search string that should be used to filter through available
    // frames that can be joined to (#21).
    std::string m_JoinToSearchFilter;

    // details of the to-be-added body
    BodyDetails m_BodyDetails;
};


osc::AddBodyPopup::AddBodyPopup(
    Widget* parent,
    std::string_view popupName,
    std::shared_ptr<IModelStatePair> modelState) :

    Popup{std::make_unique<Impl>(*this, parent, popupName, std::move(modelState))}
{}
void osc::AddBodyPopup::impl_draw_content() { private_data().draw_content(); }
void osc::AddBodyPopup::impl_on_close() { private_data().on_close(); }
