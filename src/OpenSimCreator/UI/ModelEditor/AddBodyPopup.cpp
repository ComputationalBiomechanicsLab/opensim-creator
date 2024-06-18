#include "AddBodyPopup.h"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/UI/ModelEditor/SelectGeometryPopup.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Ground.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>

#include <memory>
#include <span>
#include <string>
#include <utility>


class osc::AddBodyPopup::Impl final : public StandardPopup {
public:
    Impl(std::string_view popupName,
         IEditorAPI* api,
         std::shared_ptr<UndoableModelStatePair> uum) :

        StandardPopup{popupName},
        m_EditorAPI{api},
        m_Uum{std::move(uum)}
    {
    }

private:
    void impl_draw_content() final
    {
        const OpenSim::Model& model = m_Uum->getModel();

        auto const* selectedPf = FindComponent<OpenSim::PhysicalFrame>(model, m_BodyDetails.parentFrameAbsPath);
        if (!selectedPf)
        {
            // if nothing selected (or not found), coerce the initial selection to ground
            selectedPf = &model.getGround();
            m_BodyDetails.parentFrameAbsPath = GetAbsolutePathString(*selectedPf);
        }

        ui::set_num_columns(2);

        // prompt name
        {
            if (is_popup_opened_this_frame())
            {
                ui::set_keyboard_focus_here();
            }

            ui::draw_text("body name");
            ui::same_line();
            ui::draw_help_marker("The name used to identify the OpenSim::Body in the model. OpenSim typically uses the name to identify connections between components in a model, so the name should be unique.");
            ui::next_column();
            ui::set_next_item_width(ui::get_content_region_avail().x);
            ui::draw_string_input("##bodyname", m_BodyDetails.bodyName);
            App::upd().add_frame_annotation("AddBodyPopup::BodyNameInput", ui::get_last_drawn_item_screen_rect());
            ui::next_column();
        }

        // prompt mass
        {
            ui::draw_text("mass (kg)");
            ui::same_line();
            ui::draw_help_marker("The mass of the body in kilograms");
            ui::next_column();
            ui::set_next_item_width(ui::get_content_region_avail().x);
            ui::draw_float_kilogram_input("##mass", m_BodyDetails.mass);
            ui::next_column();
        }

        // prompt center of mass
        {
            ui::draw_text("center of mass");
            ui::same_line();
            ui::draw_help_marker("The location of the mass center in the body frame.");
            ui::next_column();
            ui::set_next_item_width(ui::get_content_region_avail().x);
            ui::draw_float3_meters_input("##comeditor", m_BodyDetails.centerOfMass);
            ui::next_column();
        }

        // prompt inertia
        {
            ui::draw_text("inertia (tensor)");
            ui::same_line();
            ui::draw_help_marker("The elements of the inertia tensor (Vec6) as [Ixx Iyy Izz Ixy Ixz Iyz]. These are measured about the center of mass, *not* the center of the body frame.");
            ui::next_column();
            ui::set_next_item_width(ui::get_content_region_avail().x);
            ui::draw_float3_meters_input("##inertiaeditor", m_BodyDetails.inertia);
            ui::next_column();
        }

        // prompt body/ground that new body will connect to (via a joint)
        {
            ui::draw_text("join to");
            ui::same_line();
            ui::draw_help_marker("What the added body will be joined to. All bodies in an OpenSim model are connected to other bodies, or the ground, by joints. This is true even if the joint is unconstrained and does nothing (e.g. an OpenSim::FreeJoint) or if the joint constrains motion in all direcctions (e.g. an OpenSim::WeldJoint).");
            ui::next_column();

            ui::begin_child_panel("join targets", Vec2{0, 128.0f}, ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar);
            for (const OpenSim::PhysicalFrame& pf : model.getComponentList<OpenSim::PhysicalFrame>())
            {
                if (ui::draw_selectable(pf.getName(), &pf == selectedPf))
                {
                    selectedPf = &pf;
                    m_BodyDetails.parentFrameAbsPath = GetAbsolutePathString(*selectedPf);
                }
                if (&pf == selectedPf)
                {
                    App::upd().add_frame_annotation(pf.getName(), ui::get_last_drawn_item_screen_rect());
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
                App::upd().add_frame_annotation("AddBodyPopup::JointTypeInput", ui::get_last_drawn_item_screen_rect());
            }
            ui::next_column();
        }

        // prompt joint name
        {
            ui::draw_text("joint name");
            ui::same_line();
            ui::draw_help_marker("The name of the OpenSim::Joint that will join the new body to the existing frame specified above");
            ui::next_column();
            ui::set_next_item_width(ui::get_content_region_avail().x);
            ui::draw_string_input("##jointnameinput", m_BodyDetails.jointName);
            App::upd().add_frame_annotation("AddBodyPopup::JointNameInput", ui::get_last_drawn_item_screen_rect());
            ui::next_column();
        }

        // prompt adding offset frames
        {
            ui::draw_text("add offset frames");
            ui::same_line();
            ui::draw_help_marker("Whether osc should automatically add intermediate offset frames to the OpenSim::Joint. A joint can attach to the two bodies (this added one, plus the selected one) directly. However, many OpenSim model designs instead make the joint attach to offset frames which, themselves, attach to the bodies. The utility of doing this is that the offset frames can be manually adjusted later, rather than *having* to attach the center of the joint to the center of the body");
            ui::next_column();
            ui::draw_checkbox("##addoffsetframescheckbox", &m_BodyDetails.addOffsetFrames);
            App::upd().add_frame_annotation("AddBodyPopup::AddOffsetFramesInput", ui::get_last_drawn_item_screen_rect());
            ui::next_column();
        }

        // prompt geometry
        {
            ui::draw_text("geometry");
            ui::same_line();
            ui::draw_help_marker("Attaches visual geometry to the new body. This is what the OpenSim::Body looks like in the UI. The geometry is purely cosmetic and does not affect the simulation");
            ui::next_column();
            {
                std::string label = m_BodyDetails.maybeGeometry ? GetDisplayName(*m_BodyDetails.maybeGeometry) : std::string{"attach"};

                if (ui::draw_button(label))
                {
                    // open geometry selection popup
                    auto popup = std::make_unique<SelectGeometryPopup>(
                        "addbody_attachgeometry",
                        App::resource_filepath("geometry"),
                        [this](auto ptr) { onGeometrySelection(std::move(ptr)); });
                    popup->open();
                    m_EditorAPI->pushPopup(std::move(popup));
                }
                App::upd().add_frame_annotation("AddBodyPopup::GeometryButton", ui::get_last_drawn_item_screen_rect());
            }
            ui::next_column();
        }

        ui::set_num_columns();

        // end of input prompting: show user cancel/ok buttons

        ui::draw_dummy({0.0f, 1.0f});

        if (ui::draw_button("cancel"))
        {
            request_close();
        }

        ui::same_line();

        if (ui::draw_button(ICON_FA_PLUS " add body"))
        {
            ActionAddBodyToModel(*m_Uum, m_BodyDetails);
            request_close();
        }
    }

    void impl_on_close() final
    {
        m_BodyDetails = BodyDetails{};
    }

    void onGeometrySelection(std::unique_ptr<OpenSim::Geometry> ptr)
    {
        m_BodyDetails.maybeGeometry = std::move(ptr);
    }

    // ability to push popups to the main UI
    IEditorAPI* m_EditorAPI;

    // the model that the body will be added to
    std::shared_ptr<UndoableModelStatePair> m_Uum;

    // details of the to-be-added body
    BodyDetails m_BodyDetails;
};


// public API

osc::AddBodyPopup::AddBodyPopup(
    std::string_view popupName,
    IEditorAPI* api,
    std::shared_ptr<UndoableModelStatePair> uum) :

    m_Impl{std::make_unique<Impl>(popupName, api, std::move(uum))}
{
}
osc::AddBodyPopup::AddBodyPopup(AddBodyPopup&&) noexcept = default;
osc::AddBodyPopup& osc::AddBodyPopup::operator=(AddBodyPopup&&) noexcept = default;
osc::AddBodyPopup::~AddBodyPopup() noexcept = default;

bool osc::AddBodyPopup::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::AddBodyPopup::impl_open()
{
    m_Impl->open();
}

void osc::AddBodyPopup::impl_close()
{
    m_Impl->close();
}

bool osc::AddBodyPopup::impl_begin_popup()
{
    return m_Impl->begin_popup();
}

void osc::AddBodyPopup::impl_on_draw()
{
    m_Impl->on_draw();
}

void osc::AddBodyPopup::impl_end_popup()
{
    m_Impl->end_popup();
}
