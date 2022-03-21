#include "ModelActionsMenuBar.hpp"

#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UiModel.hpp"
#include "src/Platform/Log.hpp"
#include "src/Widgets/AddBodyPopup.hpp"
#include "src/Widgets/AddComponentPopup.hpp"
#include "src/Widgets/Select2PFsPopup.hpp"

#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <optional>
#include <utility>

static void drawTooltip(char const* header, char const* description)
{
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(header);
    ImGui::Dummy({0.0f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.7f, 0.7f, 0.7f, 1.0f});
    ImGui::TextUnformatted(description);
    ImGui::PopStyleColor();
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

class osc::ModelActionsMenuBar::Impl final {
public:
    Impl() :
        abm{},
        select2PFsPopup{},
        jointIndexForPFsPopup{-1},
        addComponentPopupName{nullptr},
        addComponentPopup{std::nullopt}
    {
    }

    AddBodyPopup abm;
    Select2PFsPopup select2PFsPopup;
    int jointIndexForPFsPopup;
    char const* addComponentPopupName;
    std::optional<AddComponentPopup> addComponentPopup;
};

static bool renderModelActionsPanelContent(osc::ModelActionsMenuBar::Impl& st,
                                           osc::UiModel& uim)
{
    bool editMade = false;

    // action: add body
    {
        static constexpr char const* addBodyPopupName = "add body";

        // draw button
        if (ImGui::MenuItem(ICON_FA_PLUS " add body"))
        {
            ImGui::OpenPopup(addBodyPopupName);
        }

        // draw tooltip (if hovered)
        if (ImGui::IsItemHovered())
        {
            drawTooltip(
                "Add an OpenSim::Body into the model",
                "An OpenSim::Body is a PhysicalFrame (reference frame) with an associated inertia specified by its mass, center-of-mass located in the PhysicalFrame, and its moment of inertia tensor about the center-of-mass");
        }

        // draw popup (if requested)
        if (auto maybeNewBody = st.abm.draw(addBodyPopupName, uim.getModel()); maybeNewBody)
        {
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
            auto jointNames = osc::JointRegistry::nameCStrings();

            for (size_t i = 0; i < jointNames.size(); ++i)
            {
                if (ImGui::MenuItem(jointNames[i]))
                {
                    std::unique_ptr<OpenSim::Joint> copy{osc::JointRegistry::prototypes()[i]->clone()};
                    st.addComponentPopup = osc::AddComponentPopup{std::move(copy)};
                    st.addComponentPopupName = "Add Joint";
                    openPopup = true;
                }

                if (ImGui::IsItemHovered())
                {
                    drawTooltip(jointNames[i], osc::JointRegistry::descriptionCStrings()[i]);
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
            auto contactGeomNames = osc::ContactGeometryRegistry::nameCStrings();

            for (size_t i = 0; i < contactGeomNames.size(); ++i) {
                if (ImGui::MenuItem(contactGeomNames[i])) {
                    std::unique_ptr<OpenSim::ContactGeometry> copy{osc::ContactGeometryRegistry::prototypes()[i]->clone()};
                    st.addComponentPopup = osc::AddComponentPopup{std::move(copy)};
                    st.addComponentPopupName = "Add Contact Geometry";
                    openPopup = true;
                }
                if (ImGui::IsItemHovered()) {
                    drawTooltip(contactGeomNames[i], osc::ContactGeometryRegistry::descriptionCStrings()[i]);
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
            auto constraintRegistryNames = osc::ConstraintRegistry::nameCStrings();
            for (size_t i = 0; i < constraintRegistryNames.size(); ++i) {

                if (ImGui::MenuItem(constraintRegistryNames[i])) {
                    std::unique_ptr<OpenSim::Constraint> copy{osc::ConstraintRegistry::prototypes()[i]->clone()};
                    st.addComponentPopup = osc::AddComponentPopup{std::move(copy)};
                    st.addComponentPopupName = "Add Constraint";
                    openPopup = true;
                }
                if (ImGui::IsItemHovered()) {
                    drawTooltip(constraintRegistryNames[i], osc::ConstraintRegistry::descriptionCStrings()[i]);
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
            auto forceRegistryNames = osc::ForceRegistry::nameCStrings();
            for (size_t i = 0; i < forceRegistryNames.size(); ++i) {

                if (ImGui::MenuItem(forceRegistryNames[i])) {
                    std::unique_ptr<OpenSim::Force> copy{osc::ForceRegistry::prototypes()[i]->clone()};
                    st.addComponentPopup = osc::AddComponentPopup{std::move(copy)};
                    st.addComponentPopupName = "Add Force";
                    openPopup = true;
                }
                if (ImGui::IsItemHovered()) {
                    drawTooltip(forceRegistryNames[i], osc::ForceRegistry::descriptionCStrings()[i]);
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
                osc::log::error(
                    "don't know how to add a component of type %s to the model",
                    newComponent->getConcreteClassName().c_str());
            }
        }
    }

    return editMade;
}

osc::ModelActionsMenuBar::ModelActionsMenuBar() : m_Impl{std::make_unique<Impl>()} {}
osc::ModelActionsMenuBar::ModelActionsMenuBar(ModelActionsMenuBar&&) noexcept = default;
osc::ModelActionsMenuBar& osc::ModelActionsMenuBar::operator=(ModelActionsMenuBar&&) noexcept = default;
osc::ModelActionsMenuBar::~ModelActionsMenuBar() noexcept = default;

bool osc::ModelActionsMenuBar::draw(UiModel& uim)
{
    if (ImGui::BeginMenuBar())
    {
        return renderModelActionsPanelContent(*m_Impl, uim);
        ImGui::EndMenuBar();
    }
    else
    {
        return false;
    }
}
