#include "ModelActionsMenuBar.hpp"

#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UiModel.hpp"
#include "src/UI/AddBodyPopup.hpp"
#include "src/UI/Select2PFsPopup.hpp"
#include "src/Log.hpp"
#include "src/Styling.hpp"

#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <imgui.h>

using namespace osc;



static void drawTooltip(char const* header, char const* description) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(header);
    ImGui::Dummy(ImVec2{0.0f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.7f, 0.7f, 0.7f, 1.0f});
    ImGui::TextUnformatted(description);
    ImGui::PopStyleColor();
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

static bool renderModelActionsPanelContent(ModelActionsMenuBar& st, UiModel& uim) {

    bool editMade = false;

    // action: add body
    {
        static constexpr char const* addBodyPopupName = "add body";

        // draw button
        if (ImGui::MenuItem(ICON_FA_PLUS " add body")) {
            ImGui::OpenPopup(addBodyPopupName);
        }

        // draw tooltip (if hovered)
        if (ImGui::IsItemHovered()) {
            drawTooltip(
                "Add an OpenSim::Body into the model",
                "An OpenSim::Body is a PhysicalFrame (reference frame) with an associated inertia specified by its mass, center-of-mass located in the PhysicalFrame, and its moment of inertia tensor about the center-of-mass");
        }

        // draw popup (if requested)
        if (auto maybeNewBody = st.abm.draw(addBodyPopupName, uim.getModel()); maybeNewBody) {
            OpenSim::Model& model = uim.updModel();
            model.addJoint(maybeNewBody->joint.release());
            auto* ptr = maybeNewBody->body.get();
            model.addBody(maybeNewBody->body.release());
            uim.setSelected(ptr);
            editMade = true;
        }
    }

    // action: add joint
    {
        bool openPopup = false;  // has to be outside ImGui::Menu

        // draw dropdown menu (containing concrete `OpenSim::ContactGeometry`s)
        if (ImGui::BeginMenu(ICON_FA_PLUS " add joint")) {
            auto jointNames = JointRegistry::nameCStrings();

            for (size_t i = 0; i < jointNames.size(); ++i) {
                if (ImGui::MenuItem(jointNames[i])) {
                    std::unique_ptr<OpenSim::Joint> copy{JointRegistry::prototypes()[i]->clone()};
                    st.addComponentPopup = AddComponentPopup{std::move(copy)};
                    st.addComponentPopupName = "Add Joint";
                    openPopup = true;
                }
                if (ImGui::IsItemHovered()) {
                    drawTooltip(jointNames[i], JointRegistry::descriptionCStrings()[i]);
                }
            }

            ImGui::EndMenu();
        }

        // draw general OpenSim::Joint tooltip (if top-level menu hovered)
        if (ImGui::IsItemHovered()) {
            drawTooltip(
                "Add an OpenSim::Joint into the model",
                "An OpenSim::Joint is a OpenSim::ModelComponent which connects two PhysicalFrames together and specifies their relative permissible motion as described in internal coordinates.");
        }

        // draw popup (if requested)
        if (openPopup) {
            ImGui::OpenPopup(st.addComponentPopupName);
        }
    }

    // action: add contact geometry
    {
        bool openPopup = false;  // has to be outside ImGui::Menu

        // draw dropdown menu (containing concrete `OpenSim::ContactGeometry`s)
        if (ImGui::BeginMenu(ICON_FA_PLUS " add contact geometry")) {
            auto contactGeomNames = ContactGeometryRegistry::nameCStrings();

            for (size_t i = 0; i < contactGeomNames.size(); ++i) {
                if (ImGui::MenuItem(contactGeomNames[i])) {
                    std::unique_ptr<OpenSim::ContactGeometry> copy{ContactGeometryRegistry::prototypes()[i]->clone()};
                    st.addComponentPopup = AddComponentPopup{std::move(copy)};
                    st.addComponentPopupName = "Add Contact Geometry";
                    openPopup = true;
                }
                if (ImGui::IsItemHovered()) {
                    drawTooltip(contactGeomNames[i], ContactGeometryRegistry::descriptionCStrings()[i]);
                }
            }

            ImGui::EndMenu();
        }

        // draw general OpenSim::ContactGeometry tooltip (if top-level menu hovered)
        if (ImGui::IsItemHovered()) {
            drawTooltip(
                "Add an OpenSim::ContactGeometry into the model",
                "Add a geometry with a physical shape that participates in contact modeling. The geometry is attached to an OpenSim::PhysicalFrame in the model (e.g. a body) and and moves with that frame.");
        }

        // draw popup (if requested)
        if (openPopup) {
            ImGui::OpenPopup(st.addComponentPopupName);
        }
    }

    // action: add constraint
    {
        bool openPopup = false;  // has to be outside ImGui::Menu

        // draw dropdown menu (containing concrete `OpenSim::Constraint`s)
        if (ImGui::BeginMenu(ICON_FA_PLUS " add constraint")) {
            auto constraintRegistryNames = ConstraintRegistry::nameCStrings();
            for (size_t i = 0; i < constraintRegistryNames.size(); ++i) {

                if (ImGui::MenuItem(constraintRegistryNames[i])) {
                    std::unique_ptr<OpenSim::Constraint> copy{ConstraintRegistry::prototypes()[i]->clone()};
                    st.addComponentPopup = AddComponentPopup{std::move(copy)};
                    st.addComponentPopupName = "Add Constraint";
                    openPopup = true;
                }
                if (ImGui::IsItemHovered()) {
                    drawTooltip(constraintRegistryNames[i], ConstraintRegistry::descriptionCStrings()[i]);
                }
            }

            ImGui::EndMenu();
        }

        // draw general OpenSim::Constraint tooltip (if top-level menu hovered)
        if (ImGui::IsItemHovered()) {
            drawTooltip(
                "Add an OpenSim::Constraint into the model",
                "Add a constraint into the model. A constraint typically constrains the motion of physical frame(s) in the model some way. For example, an OpenSim::ConstantDistanceConstraint constrains the system to *have* to keep two frames at some constant distance from eachover.");
        }

        if (openPopup) {
            ImGui::OpenPopup(st.addComponentPopupName);
        }
    }

    // action: add force
    {
        bool openPopup = false;  // has to be outside ImGui::Menu

        // draw dropdown menu (containing concrete `OpenSim::Force`s)
        if (ImGui::BeginMenu(ICON_FA_PLUS " add force/muscle")) {
            auto forceRegistryNames = ForceRegistry::nameCStrings();
            for (size_t i = 0; i < forceRegistryNames.size(); ++i) {

                if (ImGui::MenuItem(forceRegistryNames[i])) {
                    std::unique_ptr<OpenSim::Force> copy{ForceRegistry::prototypes()[i]->clone()};
                    st.addComponentPopup = AddComponentPopup{std::move(copy)};
                    st.addComponentPopupName = "Add Force";
                    openPopup = true;
                }
                if (ImGui::IsItemHovered()) {
                    drawTooltip(forceRegistryNames[i], ForceRegistry::descriptionCStrings()[i]);
                }
            }

            ImGui::EndMenu();
        }

        // draw general OpenSim::Force tooltip (if top-level menu hovered)
        if (ImGui::IsItemHovered()) {
            drawTooltip(
                "Add an OpenSim::Force into the model",
                "Add a force into the model. During a simulation, the force is applied to bodies or generalized coordinates in the model. Muscles are specialized `OpenSim::Force`s with biomech-focused features.");
        }


        if (openPopup) {
            ImGui::OpenPopup(st.addComponentPopupName);
        }
    }

    if (st.addComponentPopup && st.addComponentPopupName) {
        auto newComponent = st.addComponentPopup->draw(st.addComponentPopupName, uim.getModel());

        if (newComponent) {
            auto ptr = newComponent.get();

            if (dynamic_cast<OpenSim::Joint*>(newComponent.get())) {
                uim.updModel().addJoint(static_cast<OpenSim::Joint*>(newComponent.release()));
                uim.setSelected(ptr);
                editMade = true;
            } else if (dynamic_cast<OpenSim::Force*>(newComponent.get())) {
                uim.updModel().addForce(static_cast<OpenSim::Force*>(newComponent.release()));
                uim.setSelected(ptr);
                editMade = true;
            } else if (dynamic_cast<OpenSim::Constraint*>(newComponent.get())) {
                uim.updModel().addConstraint(static_cast<OpenSim::Constraint*>(newComponent.release()));
                uim.setSelected(ptr);
                editMade = true;
            } else if (dynamic_cast<OpenSim::ContactGeometry*>(newComponent.get())) {
                uim.updModel().addContactGeometry(static_cast<OpenSim::ContactGeometry*>(newComponent.release()));
                uim.setSelected(ptr);
                editMade = true;
            } else {
                log::error(
                    "don't know how to add a component of type %s to the model",
                    newComponent->getConcreteClassName().c_str());
            }
        }
    }

    return editMade;
}

osc::ModelActionsMenuBar::ModelActionsMenuBar() :
    abm{},
    select2PFsPopup{},
    jointIndexForPFsPopup{-1},
    addComponentPopupName{nullptr},
    addComponentPopup{std::nullopt} {
}

bool osc::ModelActionsMenuBar::draw(UiModel& uim) {
    if (ImGui::BeginMenuBar()) {
        return renderModelActionsPanelContent(*this, uim);
        ImGui::EndMenuBar();
    } else {
        return false;
    }
}
