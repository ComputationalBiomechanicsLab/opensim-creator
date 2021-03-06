#include "AddBodyPopup.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Widgets/SelectGeometryPopup.hpp"
#include "src/Widgets/StandardPopup.hpp"

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

static inline constexpr int g_MaxBodyNameLength = 128;
static inline constexpr int g_MaxJointNameLength = 128;

class osc::AddBodyPopup::Impl : public osc::StandardPopup {
public:
    Impl(std::shared_ptr<UndoableModelStatePair> uum, std::string_view popupName) :
        StandardPopup{std::move(popupName)},
        m_Uum{std::move(uum)}
    {
    }

private:
    void implDraw() override
    {
        OpenSim::Model const& model = m_Uum->getModel();

        OpenSim::PhysicalFrame const* selectedPf =
            osc::FindComponent<OpenSim::PhysicalFrame>(model, m_BodyDetails.ParentFrameAbsPath);

        if (!selectedPf)
        {
            // if nothing selected (or not found), coerce the initial selection to ground
            selectedPf = &model.getGround();
            m_BodyDetails.ParentFrameAbsPath = selectedPf->getAbsolutePathString();
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
            osc::InputString("##bodyname", m_BodyDetails.BodyName, g_MaxBodyNameLength);
            ImGui::NextColumn();
        }

        // prompt mass
        {
            ImGui::Text("mass (kg)");
            ImGui::SameLine();
            DrawHelpMarker("The mass of the body in kilograms");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            InputKilogramFloat("##mass", &m_BodyDetails.Mass);
            ImGui::NextColumn();
        }

        // prompt center of mass
        {
            ImGui::Text("center of mass");
            ImGui::SameLine();
            DrawHelpMarker("The location of the mass center in the body frame.");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            osc::InputMetersFloat3("##comeditor", glm::value_ptr(m_BodyDetails.CenterOfMass));
            ImGui::NextColumn();
        }

        // prompt inertia
        {
            ImGui::Text("inertia (tensor)");
            ImGui::SameLine();
            DrawHelpMarker("The elements of the inertia tensor (Vec6) as [Ixx Iyy Izz Ixy Ixz Iyz]. These are measured about the center of mass, *not* the center of the body frame.");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            osc::InputMetersFloat3("##inertiaeditor", glm::value_ptr(m_BodyDetails.Inertia));
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
                    m_BodyDetails.ParentFrameAbsPath = selectedPf->getAbsolutePathString();
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
                auto names = osc::JointRegistry::nameCStrings();
                ImGui::Combo("##jointtype", &m_BodyDetails.JointTypeIndex, names.data(), static_cast<int>(names.size()));
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
            osc::InputString("##jointnameinput", m_BodyDetails.JointName, g_MaxJointNameLength);
            ImGui::NextColumn();
        }

        // prompt adding offset frames
        {
            ImGui::Text("add offset frames");
            ImGui::SameLine();
            DrawHelpMarker("Whether osc should automatically add intermediate offset frames to the OpenSim::Joint. A joint can attach to the two bodies (this added one, plus the selected one) directly. However, many OpenSim model designs instead make the joint attach to offset frames which, themselves, attach to the bodies. The utility of doing this is that the offset frames can be manually adjusted later, rather than *having* to attach the center of the joint to the center of the body");
            ImGui::NextColumn();
            ImGui::Checkbox("##addoffsetframescheckbox", &m_BodyDetails.AddOffsetFrames);
            ImGui::NextColumn();
        }

        // prompt geometry
        {
            ImGui::Text("geometry");
            ImGui::SameLine();
            DrawHelpMarker("Attaches visual geometry to the new body. This is what the OpenSim::Body looks like in the UI. The geometry is purely cosmetic and does not affect the simulation");
            ImGui::NextColumn();
            {
                std::string label = m_BodyDetails.MaybeGeometry ? GetDisplayName(*m_BodyDetails.MaybeGeometry) : std::string{"attach"};

                if (ImGui::Button(label.c_str()))
                {
                    m_AttachGeometryPopup.open();
                }

                if (auto attached = m_AttachGeometryPopup.draw(); attached)
                {
                    m_BodyDetails.MaybeGeometry = std::move(attached);
                }
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

    void implOnClose() override
    {
        m_BodyDetails = BodyDetails{};
    }

    // the model that the body will be added to
    std::shared_ptr<UndoableModelStatePair> m_Uum;

    // state for the "attach geometry" popup
    SelectGeometryPopup m_AttachGeometryPopup{"addbody_attachgeometry"};

    // details of the to-be-added body
    BodyDetails m_BodyDetails;
};


// public API

osc::AddBodyPopup::AddBodyPopup(std::shared_ptr<UndoableModelStatePair> uum,
                                std::string_view popupName) :
    m_Impl{new Impl{std::move(uum), std::move(popupName)}}
{
}
osc::AddBodyPopup::AddBodyPopup(AddBodyPopup&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::AddBodyPopup& osc::AddBodyPopup::operator=(AddBodyPopup&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::AddBodyPopup::~AddBodyPopup() noexcept
{
    delete m_Impl;
}

void osc::AddBodyPopup::open()
{
    m_Impl->open();
}

void osc::AddBodyPopup::close()
{
    m_Impl->close();
}

void osc::AddBodyPopup::draw()
{
    m_Impl->draw();
}
