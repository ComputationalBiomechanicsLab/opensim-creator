#include "ModelActionsMenuBar.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UndoableUiModel.hpp"
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

class osc::ModelActionsMenuBar::Impl final {
public:
    Impl(std::shared_ptr<UndoableUiModel> uum_) :
        m_Uum{uum_},
        m_AddBodyPopup{uum_, "add body"},
        m_Select2PFsPopup{},
        m_MaybeAddComponentPopup{std::nullopt}
    {
    }

    bool draw()
    {
        if (ImGui::BeginMenuBar())
        {
            return renderMenuBarContent();
            ImGui::EndMenuBar();
        }
        else
        {
            return false;
        }
    }

private:
    bool renderMenuBarContent()
    {
        bool editMade = false;

        // action: add body
        {
            // draw button
            if (ImGui::MenuItem(ICON_FA_PLUS " add body"))
            {
                m_AddBodyPopup.open();
            }

            // draw tooltip (if hovered)
            if (ImGui::IsItemHovered())
            {
                DrawTooltip(
                    "Add an OpenSim::Body into the model",
                    "An OpenSim::Body is a PhysicalFrame (reference frame) with an associated inertia specified by its mass, center-of-mass located in the PhysicalFrame, and its moment of inertia tensor about the center-of-mass");
            }

            if (m_AddBodyPopup.draw())
            {
                editMade = true;
            }
        }

        // action: add joint
        if (ImGui::BeginMenu(ICON_FA_PLUS " add joint"))
        {
            auto jointNames = osc::JointRegistry::nameCStrings();

            for (size_t i = 0; i < jointNames.size(); ++i)
            {
                if (ImGui::MenuItem(jointNames[i]))
                {
                    std::unique_ptr<OpenSim::Joint> copy{osc::JointRegistry::prototypes()[i]->clone()};
                    m_MaybeAddComponentPopup = osc::AddComponentPopup{m_Uum, std::move(copy), "Add Joint"};
                    m_MaybeAddComponentPopup->open();
                }

                if (ImGui::IsItemHovered())
                {
                    DrawTooltip(jointNames[i], osc::JointRegistry::descriptionCStrings()[i]);
                }
            }

            ImGui::EndMenu();
        }
        if (ImGui::IsItemHovered())
        {
            DrawTooltip(
                "Add an OpenSim::Joint into the model",
                "An OpenSim::Joint is a OpenSim::ModelComponent which connects two PhysicalFrames together and specifies their relative permissible motion as described in internal coordinates.");
        }

        // action: add contact geometry
        if (ImGui::BeginMenu(ICON_FA_PLUS " add contact geometry"))
        {
            auto contactGeomNames = osc::ContactGeometryRegistry::nameCStrings();

            for (size_t i = 0; i < contactGeomNames.size(); ++i)
            {
                if (ImGui::MenuItem(contactGeomNames[i]))
                {
                    std::unique_ptr<OpenSim::ContactGeometry> copy{osc::ContactGeometryRegistry::prototypes()[i]->clone()};
                    m_MaybeAddComponentPopup = osc::AddComponentPopup{m_Uum, std::move(copy), "Add Contact Geometry"};
                    m_MaybeAddComponentPopup->open();
                }
                if (ImGui::IsItemHovered())
                {
                    DrawTooltip(contactGeomNames[i], osc::ContactGeometryRegistry::descriptionCStrings()[i]);
                }
            }

            ImGui::EndMenu();
        }
        if (ImGui::IsItemHovered())
        {
            DrawTooltip(
                "Add an OpenSim::ContactGeometry into the model",
                "Add a geometry with a physical shape that participates in contact modeling. The geometry is attached to an OpenSim::PhysicalFrame in the model (e.g. a body) and and moves with that frame.");
        }

        // action: add constraint
        if (ImGui::BeginMenu(ICON_FA_PLUS " add constraint"))
        {
            auto constraintRegistryNames = osc::ConstraintRegistry::nameCStrings();
            for (size_t i = 0; i < constraintRegistryNames.size(); ++i)
            {
                if (ImGui::MenuItem(constraintRegistryNames[i]))
                {
                    std::unique_ptr<OpenSim::Constraint> copy{osc::ConstraintRegistry::prototypes()[i]->clone()};
                    m_MaybeAddComponentPopup = osc::AddComponentPopup{m_Uum, std::move(copy), "Add Constraint"};
                    m_MaybeAddComponentPopup->open();
                }
                if (ImGui::IsItemHovered())
                {
                    DrawTooltip(constraintRegistryNames[i], osc::ConstraintRegistry::descriptionCStrings()[i]);
                }
            }

            ImGui::EndMenu();
        }
        if (ImGui::IsItemHovered())
        {
            DrawTooltip(
                "Add an OpenSim::Constraint into the model",
                "Add a constraint into the model. A constraint typically constrains the motion of physical frame(s) in the model some way. For example, an OpenSim::ConstantDistanceConstraint constrains the system to *have* to keep two frames at some constant distance from eachover.");
        }

        // draw dropdown menu (containing concrete `OpenSim::Force`s)
        if (ImGui::BeginMenu(ICON_FA_PLUS " add force/muscle"))
        {
            auto forceRegistryNames = osc::ForceRegistry::nameCStrings();
            for (size_t i = 0; i < forceRegistryNames.size(); ++i)
            {
                if (ImGui::MenuItem(forceRegistryNames[i]))
                {
                    std::unique_ptr<OpenSim::Force> copy{osc::ForceRegistry::prototypes()[i]->clone()};
                    m_MaybeAddComponentPopup = osc::AddComponentPopup{m_Uum, std::move(copy), "Add Force"};
                    m_MaybeAddComponentPopup->open();
                }
                if (ImGui::IsItemHovered())
                {
                    DrawTooltip(forceRegistryNames[i], osc::ForceRegistry::descriptionCStrings()[i]);
                }
            }

            ImGui::EndMenu();
        }
        if (ImGui::IsItemHovered())
        {
            DrawTooltip(
                "Add an OpenSim::Force into the model",
                "Add a force into the model. During a simulation, the force is applied to bodies or generalized coordinates in the model. Muscles are specialized `OpenSim::Force`s with biomech-focused features.");
        }

        // draw dropdown menu (containing concrete `OpenSim::Force`s)
        if (ImGui::BeginMenu(ICON_FA_PLUS " add controller"))
        {
            auto forceRegistryNames = osc::ControllerRegistry::nameCStrings();
            for (size_t i = 0; i < forceRegistryNames.size(); ++i)
            {
                if (ImGui::MenuItem(forceRegistryNames[i]))
                {
                    std::unique_ptr<OpenSim::Controller> copy{osc::ControllerRegistry::prototypes()[i]->clone()};
                    m_MaybeAddComponentPopup = osc::AddComponentPopup{m_Uum, std::move(copy), "Add Controller"};
                    m_MaybeAddComponentPopup->open();
                }
                if (ImGui::IsItemHovered())
                {
                    DrawTooltip(forceRegistryNames[i], osc::ControllerRegistry::descriptionCStrings()[i]);
                }
            }

            ImGui::EndMenu();
        }
        if (ImGui::IsItemHovered())
        {
            DrawTooltip(
                "Add an OpenSim::Controller into the model",
                "Add a controller into the model.");
        }

        if (m_MaybeAddComponentPopup)
        {
            editMade = m_MaybeAddComponentPopup->draw();
        }

        return editMade;
    }

    std::shared_ptr<UndoableUiModel> m_Uum;
    AddBodyPopup m_AddBodyPopup;
    Select2PFsPopup m_Select2PFsPopup;
    std::optional<AddComponentPopup> m_MaybeAddComponentPopup;
};

osc::ModelActionsMenuBar::ModelActionsMenuBar(std::shared_ptr<UndoableUiModel> m) :
    m_Impl{std::make_unique<Impl>(std::move(m))}
{
}
osc::ModelActionsMenuBar::ModelActionsMenuBar(ModelActionsMenuBar&&) noexcept = default;
osc::ModelActionsMenuBar& osc::ModelActionsMenuBar::operator=(ModelActionsMenuBar&&) noexcept = default;
osc::ModelActionsMenuBar::~ModelActionsMenuBar() noexcept = default;

bool osc::ModelActionsMenuBar::draw()
{
    return m_Impl->draw();
}
