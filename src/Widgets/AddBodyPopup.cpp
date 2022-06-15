#include "AddBodyPopup.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Bindings/SimTKHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Widgets/SelectComponentPopup.hpp"
#include "src/Widgets/SelectGeometryPopup.hpp"
#include "src/Widgets/StandardPopup.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <SimTKcommon.h>
#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <array>
#include <memory>
#include <optional>
#include <utility>


class osc::AddBodyPopup::Impl : public osc::StandardPopup {
public:
    Impl(std::shared_ptr<UndoableModelStatePair> uum, std::string_view popupName) :
        StandardPopup{std::move(popupName)},
        m_Uum{std::move(uum)}
    {
    }

    bool drawAndCheck()
    {
        draw();
        return std::exchange(m_BodyAddedLastDrawcall, false);
    }

private:
    void implDraw() override
    {
        OpenSim::Model const& model = m_Uum->getModel();

        OpenSim::PhysicalFrame const* selectedPf =
            osc::FindComponent<OpenSim::PhysicalFrame>(model, m_SelectedPhysicalFrameAbspath);

        if (!selectedPf)
        {
            // if nothing selected (or not found), coerce the initial selection to ground
            selectedPf = &model.getGround();
            m_SelectedPhysicalFrameAbspath = selectedPf->getAbsolutePath();
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
            osc::InputString("##bodyname", m_BodyName, m_MaxBodyNameLength);
            ImGui::NextColumn();
        }

        // prompt mass
        {
            ImGui::Text("mass (kg)");
            ImGui::SameLine();
            DrawHelpMarker("The mass of the body in kilograms");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            InputKilogramFloat("##mass", &m_BodyMass);
            ImGui::NextColumn();
        }

        // prompt center of mass
        {
            ImGui::Text("center of mass");
            ImGui::SameLine();
            DrawHelpMarker("The location of the mass center in the body frame.");
            ImGui::NextColumn();
            DrawF3Editor("##comlockbtn", "##comeditor", m_BodyCenterOfMass, &m_CenterOfMassLocked);
            ImGui::NextColumn();
        }

        // prompt inertia
        {
            ImGui::Text("inertia (tensor)");
            ImGui::SameLine();
            DrawHelpMarker("The elements of the inertia tensor (Vec6) as [Ixx Iyy Izz Ixy Ixz Iyz]. These are measured about the center of mass, *not* the center of the body frame.");
            ImGui::NextColumn();
            DrawF3Editor("##inertialockbtn", "##intertiaeditor", m_BodyInertia, &m_InertiaLocked);
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
                    m_SelectedPhysicalFrameAbspath = selectedPf->getAbsolutePath();
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
                ImGui::Combo("##jointtype", &m_JointRegistryIndex, names.data(), static_cast<int>(names.size()));
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
            osc::InputString("##jointnameinput", m_JointName, m_MaxJointNameLength);
            ImGui::NextColumn();
        }

        // prompt adding offset frames
        {
            ImGui::Text("add offset frames");
            ImGui::SameLine();
            DrawHelpMarker("Whether osc should automatically add intermediate offset frames to the OpenSim::Joint. A joint can attach to the two bodies (this added one, plus the selected one) directly. However, many OpenSim model designs instead make the joint attach to offset frames which, themselves, attach to the bodies. The utility of doing this is that the offset frames can be manually adjusted later, rather than *having* to attach the center of the joint to the center of the body");
            ImGui::NextColumn();
            ImGui::Checkbox("##addoffsetframescheckbox", &m_AddOffsetFramesToTheJoint);
            ImGui::NextColumn();
        }

        // prompt geometry
        {
            ImGui::Text("geometry");
            ImGui::SameLine();
            DrawHelpMarker("Attaches visual geometry to the new body. This is what the OpenSim::Body looks like in the UI. The geometry is purely cosmetic and does not affect the simulation");
            ImGui::NextColumn();
            {
                char const* label = "attach";
                if (m_AttachGeometryPopupSelection)
                {
                    OpenSim::Geometry const& attached = *m_AttachGeometryPopupSelection;
                    if (OpenSim::Mesh const* mesh = dynamic_cast<OpenSim::Mesh const*>(&attached); mesh)
                    {
                        label = mesh->getGeometryFilename().c_str();
                    }
                    else
                    {
                        label = attached.getConcreteClassName().c_str();
                    }
                }

                if (ImGui::Button(label))
                {
                    m_AttachGeometryPopup.open();
                }

                if (auto attached = m_AttachGeometryPopup.draw(); attached)
                {
                    m_AttachGeometryPopupSelection = std::move(attached);
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
            doAddBody(*selectedPf);
            m_BodyAddedLastDrawcall = true;
            requestClose();
        }
    }

    void implOnClose() override
    {
        // TODO: reset inputs
    }

    void doAddBody(OpenSim::PhysicalFrame const& selectedPf)
    {
        // create user-requested body
        SimTK::Vec3 com = ToSimTKVec3(m_BodyCenterOfMass);
        SimTK::Inertia inertia = ToSimTKInertia(m_BodyInertia);
        double mass = static_cast<double>(m_BodyMass);
        auto body = std::make_unique<OpenSim::Body>(m_BodyName, m_BodyMass, com, inertia);

        // create joint between body and whatever the frame is
        OpenSim::Joint const& jointProto =
            *osc::JointRegistry::prototypes()[static_cast<size_t>(m_JointRegistryIndex)];
        auto joint = makeJoint(*body, jointProto, selectedPf);

        // attach decorative geom
        if (m_AttachGeometryPopupSelection)
        {
            body->attachGeometry(m_AttachGeometryPopupSelection.release());
        }

        // mutate the model and perform the edit
        OpenSim::Model& m = m_Uum->updModel();
        m.addJoint(joint.release());
        OpenSim::Body* ptr = body.get();
        m.addBody(body.release());
        m.finalizeConnections();  // ensure sockets paths are written

        m_Uum->setSelected(ptr);
        m_Uum->commit("added body");
    }

    // create a "standard" OpenSim::Joint
    std::unique_ptr<OpenSim::Joint> makeJoint(
        OpenSim::Body const& b,
        OpenSim::Joint const& jointPrototype,
        OpenSim::PhysicalFrame const& selectedPf)
    {
        std::unique_ptr<OpenSim::Joint> copy{jointPrototype.clone()};
        copy->setName(m_JointName);

        if (!m_AddOffsetFramesToTheJoint)
        {
            copy->connectSocket_parent_frame(selectedPf);
            copy->connectSocket_child_frame(b);
        }
        else
        {
            // add first offset frame as joint's parent
            {
                auto pof1 = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                pof1->setParentFrame(selectedPf);
                pof1->setName(selectedPf.getName() + "_offset");
                copy->addFrame(pof1.get());
                copy->connectSocket_parent_frame(*pof1.release());
            }

            // add second offset frame as joint's child
            {
                auto pof2 = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                pof2->setParentFrame(b);
                pof2->setName(b.getName() + "_offset");
                copy->addFrame(pof2.get());
                copy->connectSocket_child_frame(*pof2.release());
            }
        }

        return copy;
    }

    // the model that the body will be added to
    std::shared_ptr<UndoableModelStatePair> m_Uum;

    // set when the body is added
    bool m_BodyAddedLastDrawcall = false;

    // state for the "attach geometry" popup
    SelectGeometryPopup m_AttachGeometryPopup{"addbody_attachgeometry"};
    std::unique_ptr<OpenSim::Geometry> m_AttachGeometryPopupSelection = nullptr;

    // abspath to the currently-selected physical frame in the model
    OpenSim::ComponentPath m_SelectedPhysicalFrameAbspath;

    // desired name of the body
    std::string m_BodyName{"new_body"};
    static inline constexpr int m_MaxBodyNameLength = 128;

    // index of the desired joint *type* within the type registry
    int m_JointRegistryIndex = 0;

    // desired name of the joint
    std::string m_JointName;
    static inline constexpr int m_MaxJointNameLength = 128;

    // desired mass of the to-be-added body
    float m_BodyMass = 1.0f;

    // desired center of mass of the body
    float m_BodyCenterOfMass[3]{};

    // `true` if center of mass inputs should be locked together
    bool m_CenterOfMassLocked = true;

    // desired intertia of the body
    float m_BodyInertia[3] = {1.0f, 1.0f, 1.0f};

    // `true` if the inertia inputs should be locked together
    bool m_InertiaLocked = true;

    // `true` if offset frames should be attached to the joint
    bool m_AddOffsetFramesToTheJoint = true;
};

// public API

osc::AddBodyPopup::AddBodyPopup(std::shared_ptr<UndoableModelStatePair> uum,
                                std::string_view popupName) :
    m_Impl{std::make_unique<Impl>(std::move(uum), std::move(popupName))}
{
}
osc::AddBodyPopup::AddBodyPopup(AddBodyPopup&&) noexcept = default;
osc::AddBodyPopup& osc::AddBodyPopup::operator=(AddBodyPopup&&) noexcept = default;
osc::AddBodyPopup::~AddBodyPopup() noexcept = default;

void osc::AddBodyPopup::open()
{
    m_Impl->open();
}

void osc::AddBodyPopup::close()
{
    m_Impl->close();
}

bool osc::AddBodyPopup::draw()
{
    return m_Impl->drawAndCheck();
}
