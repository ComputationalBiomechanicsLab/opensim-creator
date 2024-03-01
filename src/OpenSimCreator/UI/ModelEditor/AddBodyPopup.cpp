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
    void implDrawContent() final
    {
        OpenSim::Model const& model = m_Uum->getModel();

        auto const* selectedPf = FindComponent<OpenSim::PhysicalFrame>(model, m_BodyDetails.parentFrameAbsPath);
        if (!selectedPf)
        {
            // if nothing selected (or not found), coerce the initial selection to ground
            selectedPf = &model.getGround();
            m_BodyDetails.parentFrameAbsPath = GetAbsolutePathString(*selectedPf);
        }

        ui::Columns(2);

        // prompt name
        {
            if (isPopupOpenedThisFrame())
            {
                ui::SetKeyboardFocusHere();
            }

            ui::Text("body name");
            ui::SameLine();
            ui::DrawHelpMarker("The name used to identify the OpenSim::Body in the model. OpenSim typically uses the name to identify connections between components in a model, so the name should be unique.");
            ui::NextColumn();
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            ui::InputString("##bodyname", m_BodyDetails.bodyName);
            App::upd().addFrameAnnotation("AddBodyPopup::BodyNameInput", ui::GetItemRect());
            ui::NextColumn();
        }

        // prompt mass
        {
            ui::Text("mass (kg)");
            ui::SameLine();
            ui::DrawHelpMarker("The mass of the body in kilograms");
            ui::NextColumn();
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            ui::InputKilogramFloat("##mass", m_BodyDetails.mass);
            ui::NextColumn();
        }

        // prompt center of mass
        {
            ui::Text("center of mass");
            ui::SameLine();
            ui::DrawHelpMarker("The location of the mass center in the body frame.");
            ui::NextColumn();
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            ui::InputMetersFloat3("##comeditor", m_BodyDetails.centerOfMass);
            ui::NextColumn();
        }

        // prompt inertia
        {
            ui::Text("inertia (tensor)");
            ui::SameLine();
            ui::DrawHelpMarker("The elements of the inertia tensor (Vec6) as [Ixx Iyy Izz Ixy Ixz Iyz]. These are measured about the center of mass, *not* the center of the body frame.");
            ui::NextColumn();
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            ui::InputMetersFloat3("##inertiaeditor", m_BodyDetails.inertia);
            ui::NextColumn();
        }

        // prompt body/ground that new body will connect to (via a joint)
        {
            ui::Text("join to");
            ui::SameLine();
            ui::DrawHelpMarker("What the added body will be joined to. All bodies in an OpenSim model are connected to other bodies, or the ground, by joints. This is true even if the joint is unconstrained and does nothing (e.g. an OpenSim::FreeJoint) or if the joint constrains motion in all direcctions (e.g. an OpenSim::WeldJoint).");
            ui::NextColumn();

            ui::BeginChild("join targets", ImVec2(0, 128.0f), ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar);
            for (OpenSim::PhysicalFrame const& pf : model.getComponentList<OpenSim::PhysicalFrame>())
            {
                if (ui::Selectable(pf.getName(), &pf == selectedPf))
                {
                    selectedPf = &pf;
                    m_BodyDetails.parentFrameAbsPath = GetAbsolutePathString(*selectedPf);
                }
                if (&pf == selectedPf)
                {
                    App::upd().addFrameAnnotation(pf.getName(), ui::GetItemRect());
                }
            }
            ui::EndChild();
            ui::NextColumn();
        }

        // prompt joint type for the above
        {
            ui::Text("joint type");
            ui::SameLine();
            ui::DrawHelpMarker("The type of OpenSim::Joint that will connect the new OpenSim::Body to the selection above");
            ui::NextColumn();
            {
                auto const& registry = GetComponentRegistry<OpenSim::Joint>();
                ui::Combo(
                    "##jointtype",
                    &m_BodyDetails.jointTypeIndex,
                    registry.size(),
                    [&registry](size_t i) { return registry[i].name(); }
                );
                App::upd().addFrameAnnotation("AddBodyPopup::JointTypeInput", ui::GetItemRect());
            }
            ui::NextColumn();
        }

        // prompt joint name
        {
            ui::Text("joint name");
            ui::SameLine();
            ui::DrawHelpMarker("The name of the OpenSim::Joint that will join the new body to the existing frame specified above");
            ui::NextColumn();
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            ui::InputString("##jointnameinput", m_BodyDetails.jointName);
            App::upd().addFrameAnnotation("AddBodyPopup::JointNameInput", ui::GetItemRect());
            ui::NextColumn();
        }

        // prompt adding offset frames
        {
            ui::Text("add offset frames");
            ui::SameLine();
            ui::DrawHelpMarker("Whether osc should automatically add intermediate offset frames to the OpenSim::Joint. A joint can attach to the two bodies (this added one, plus the selected one) directly. However, many OpenSim model designs instead make the joint attach to offset frames which, themselves, attach to the bodies. The utility of doing this is that the offset frames can be manually adjusted later, rather than *having* to attach the center of the joint to the center of the body");
            ui::NextColumn();
            ui::Checkbox("##addoffsetframescheckbox", &m_BodyDetails.addOffsetFrames);
            App::upd().addFrameAnnotation("AddBodyPopup::AddOffsetFramesInput", ui::GetItemRect());
            ui::NextColumn();
        }

        // prompt geometry
        {
            ui::Text("geometry");
            ui::SameLine();
            ui::DrawHelpMarker("Attaches visual geometry to the new body. This is what the OpenSim::Body looks like in the UI. The geometry is purely cosmetic and does not affect the simulation");
            ui::NextColumn();
            {
                std::string label = m_BodyDetails.maybeGeometry ? GetDisplayName(*m_BodyDetails.maybeGeometry) : std::string{"attach"};

                if (ui::Button(label))
                {
                    // open geometry selection popup
                    auto popup = std::make_unique<SelectGeometryPopup>(
                        "addbody_attachgeometry",
                        App::resourceFilepath("geometry"),
                        [this](auto ptr) { onGeometrySelection(std::move(ptr)); });
                    popup->open();
                    m_EditorAPI->pushPopup(std::move(popup));
                }
                App::upd().addFrameAnnotation("AddBodyPopup::GeometryButton", ui::GetItemRect());
            }
            ui::NextColumn();
        }

        ui::Columns();

        // end of input prompting: show user cancel/ok buttons

        ui::Dummy({0.0f, 1.0f});

        if (ui::Button("cancel"))
        {
            requestClose();
        }

        ui::SameLine();

        if (ui::Button(ICON_FA_PLUS " add body"))
        {
            ActionAddBodyToModel(*m_Uum, m_BodyDetails);
            requestClose();
        }
    }

    void implOnClose() final
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

bool osc::AddBodyPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::AddBodyPopup::implOpen()
{
    m_Impl->open();
}

void osc::AddBodyPopup::implClose()
{
    m_Impl->close();
}

bool osc::AddBodyPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::AddBodyPopup::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::AddBodyPopup::implEndPopup()
{
    m_Impl->endPopup();
}
