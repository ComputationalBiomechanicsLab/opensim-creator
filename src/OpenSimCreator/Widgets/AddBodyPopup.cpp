#include "AddBodyPopup.hpp"

#include "OpenSimCreator/MiddlewareAPIs/EditorAPI.hpp"
#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"
#include "OpenSimCreator/Registry/ComponentRegistry.hpp"
#include "OpenSimCreator/Registry/StaticComponentRegistries.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "OpenSimCreator/Utils/UndoableModelActions.hpp"
#include "OpenSimCreator/Widgets/SelectGeometryPopup.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Widgets/StandardPopup.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Ground.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>

#include <memory>
#include <string>
#include <utility>


class osc::AddBodyPopup::Impl final : public osc::StandardPopup {
public:
    Impl(std::string_view popupName,
         EditorAPI* api,
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

        auto const* selectedPf = osc::FindComponent<OpenSim::PhysicalFrame>(model, m_BodyDetails.parentFrameAbsPath);
        if (!selectedPf)
        {
            // if nothing selected (or not found), coerce the initial selection to ground
            selectedPf = &model.getGround();
            m_BodyDetails.parentFrameAbsPath = osc::GetAbsolutePathString(*selectedPf);
        }

        ImGui::Columns(2);

        // prompt name
        {
            if (isPopupOpenedThisFrame())
            {
                ImGui::SetKeyboardFocusHere();
            }

            ImGui::Text("body name");
            ImGui::SameLine();
            DrawHelpMarker("The name used to identify the OpenSim::Body in the model. OpenSim typically uses the name to identify connections between components in a model, so the name should be unique.");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            osc::InputString("##bodyname", m_BodyDetails.bodyName);
            App::upd().addFrameAnnotation("AddBodyPopup::BodyNameInput", osc::GetItemRect());
            ImGui::NextColumn();
        }

        // prompt mass
        {
            ImGui::Text("mass (kg)");
            ImGui::SameLine();
            DrawHelpMarker("The mass of the body in kilograms");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            InputKilogramFloat("##mass", m_BodyDetails.mass);
            ImGui::NextColumn();
        }

        // prompt center of mass
        {
            ImGui::Text("center of mass");
            ImGui::SameLine();
            DrawHelpMarker("The location of the mass center in the body frame.");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            osc::InputMetersFloat3("##comeditor", m_BodyDetails.centerOfMass);
            ImGui::NextColumn();
        }

        // prompt inertia
        {
            ImGui::Text("inertia (tensor)");
            ImGui::SameLine();
            DrawHelpMarker("The elements of the inertia tensor (Vec6) as [Ixx Iyy Izz Ixy Ixz Iyz]. These are measured about the center of mass, *not* the center of the body frame.");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            osc::InputMetersFloat3("##inertiaeditor", m_BodyDetails.inertia);
            ImGui::NextColumn();
        }

        // prompt body/ground that new body will connect to (via a joint)
        {
            ImGui::Text("join to");
            ImGui::SameLine();
            DrawHelpMarker("What the added body will be joined to. All bodies in an OpenSim model are connected to other bodies, or the ground, by joints. This is true even if the joint is unconstrained and does nothing (e.g. an OpenSim::FreeJoint) or if the joint constrains motion in all direcctions (e.g. an OpenSim::WeldJoint).");
            ImGui::NextColumn();

            ImGui::BeginChild("join targets", ImVec2(0, 128.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
            for (OpenSim::PhysicalFrame const& pf : model.getComponentList<OpenSim::PhysicalFrame>())
            {
                if (ImGui::Selectable(pf.getName().c_str(), &pf == selectedPf))
                {
                    selectedPf = &pf;
                    m_BodyDetails.parentFrameAbsPath = osc::GetAbsolutePathString(*selectedPf);
                }
                if (&pf == selectedPf)
                {
                    App::upd().addFrameAnnotation(pf.getName(), osc::GetItemRect());
                }
            }
            ImGui::EndChild();
            ImGui::NextColumn();
        }

        // prompt joint type for the above
        {
            ImGui::Text("joint type");
            ImGui::SameLine();
            DrawHelpMarker("The type of OpenSim::Joint that will connect the new OpenSim::Body to the selection above");
            ImGui::NextColumn();
            {
                auto const& registry = osc::GetComponentRegistry<OpenSim::Joint>();
                osc::Combo(
                    "##jointtype",
                    &m_BodyDetails.jointTypeIndex,
                    registry.size(),
                    [&registry](size_t i) { return registry[i].name(); }
                );
                App::upd().addFrameAnnotation("AddBodyPopup::JointTypeInput", osc::GetItemRect());
            }
            ImGui::NextColumn();
        }

        // prompt joint name
        {
            ImGui::Text("joint name");
            ImGui::SameLine();
            DrawHelpMarker("The name of the OpenSim::Joint that will join the new body to the existing frame specified above");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            osc::InputString("##jointnameinput", m_BodyDetails.jointName);
            App::upd().addFrameAnnotation("AddBodyPopup::JointNameInput", osc::GetItemRect());
            ImGui::NextColumn();
        }

        // prompt adding offset frames
        {
            ImGui::Text("add offset frames");
            ImGui::SameLine();
            DrawHelpMarker("Whether osc should automatically add intermediate offset frames to the OpenSim::Joint. A joint can attach to the two bodies (this added one, plus the selected one) directly. However, many OpenSim model designs instead make the joint attach to offset frames which, themselves, attach to the bodies. The utility of doing this is that the offset frames can be manually adjusted later, rather than *having* to attach the center of the joint to the center of the body");
            ImGui::NextColumn();
            ImGui::Checkbox("##addoffsetframescheckbox", &m_BodyDetails.addOffsetFrames);
            App::upd().addFrameAnnotation("AddBodyPopup::AddOffsetFramesInput", osc::GetItemRect());
            ImGui::NextColumn();
        }

        // prompt geometry
        {
            ImGui::Text("geometry");
            ImGui::SameLine();
            DrawHelpMarker("Attaches visual geometry to the new body. This is what the OpenSim::Body looks like in the UI. The geometry is purely cosmetic and does not affect the simulation");
            ImGui::NextColumn();
            {
                std::string label = m_BodyDetails.maybeGeometry ? GetDisplayName(*m_BodyDetails.maybeGeometry) : std::string{"attach"};

                if (ImGui::Button(label.c_str()))
                {
                    // open geometry selection popup
                    auto popup = std::make_unique<SelectGeometryPopup>(
                        "addbody_attachgeometry",
                        App::resource("geometry"),
                        [this](auto ptr) { onGeometrySelection(std::move(ptr)); });
                    popup->open();
                    m_EditorAPI->pushPopup(std::move(popup));
                }
                App::upd().addFrameAnnotation("AddBodyPopup::GeometryButton", osc::GetItemRect());
            }
            ImGui::NextColumn();
        }

        ImGui::Columns();

        // end of input prompting: show user cancel/ok buttons

        ImGui::Dummy({0.0f, 1.0f});

        if (ImGui::Button("cancel"))
        {
            requestClose();
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_PLUS " add body"))
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
    EditorAPI* m_EditorAPI;

    // the model that the body will be added to
    std::shared_ptr<UndoableModelStatePair> m_Uum;

    // details of the to-be-added body
    BodyDetails m_BodyDetails;
};


// public API

osc::AddBodyPopup::AddBodyPopup(
    std::string_view popupName,
    EditorAPI* api,
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
