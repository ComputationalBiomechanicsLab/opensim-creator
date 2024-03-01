#include "ModelActionsMenuItems.h"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/UI/ModelEditor/AddBodyPopup.h>
#include <OpenSimCreator/UI/ModelEditor/AddComponentPopup.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Control/Controller.h>
#include <OpenSim/Simulation/Model/Actuator.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Probe.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

#include <concepts>
#include <sstream>
#include <utility>

class osc::ModelActionsMenuItems::Impl final {
public:

    Impl(
        IEditorAPI* api,
        std::shared_ptr<UndoableModelStatePair> uum_) :

        m_EditorAPI{api},
        m_Model{std::move(uum_)}
    {
    }

    void onDraw()
    {
        ui::PushID(this);

        // action: add body
        {
            // draw button
            if (ui::MenuItem("Body"))
            {
                auto popup = std::make_unique<AddBodyPopup>("add body", m_EditorAPI, m_Model);
                popup->open();
                m_EditorAPI->pushPopup(std::move(popup));
            }

            // draw tooltip (if hovered)
            if (ui::IsItemHovered())
            {
                ui::DrawTooltip(
                    "Add an OpenSim::Body into the model",
                    "An OpenSim::Body is a PhysicalFrame (reference frame) with an associated inertia specified by its mass, center-of-mass located in the PhysicalFrame, and its moment of inertia tensor about the center-of-mass");
            }
        }

        renderButton(GetComponentRegistry<OpenSim::Joint>());
        renderButton(GetComponentRegistry<OpenSim::ContactGeometry>());
        renderButton(GetComponentRegistry<OpenSim::Constraint>());
        renderButton(GetComponentRegistry<OpenSim::Force>());
        renderButton(GetComponentRegistry<OpenSim::Controller>());
        renderButton(GetComponentRegistry<OpenSim::Probe>());
        renderButton(GetComponentRegistry<OpenSim::Component>());
        renderButton(GetCustomComponentRegistry());

        ui::PopID();
    }

private:

    void renderButton(ComponentRegistryBase const& registry)
    {
        if (ui::BeginMenu(registry.name().c_str()))
        {
            for (auto const& entry : registry)
            {
                if (ui::MenuItem(entry.name().c_str()))
                {
                    auto popup = std::make_unique<AddComponentPopup>(
                        "Add " + registry.name(),
                        m_EditorAPI,
                        m_Model,
                        entry.instantiate()
                    );
                    popup->open();
                    m_EditorAPI->pushPopup(std::move(popup));
                }

                if (ui::IsItemHovered())
                {
                    ui::DrawTooltip(entry.name(), entry.description());
                }
            }

            ui::EndMenu();
        }

        if (ui::IsItemHovered())
        {
            std::stringstream ttTitle;
            ttTitle << "Add a " << registry.name() << " into the model";

            ui::DrawTooltip(ttTitle.str(), registry.description());
        }
    }

    IEditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;
};


// public API (PIMPL)

osc::ModelActionsMenuItems::ModelActionsMenuItems(IEditorAPI* api, std::shared_ptr<UndoableModelStatePair> m) :
    m_Impl{std::make_unique<Impl>(api, std::move(m))}
{
}

osc::ModelActionsMenuItems::ModelActionsMenuItems(ModelActionsMenuItems&&) noexcept = default;
osc::ModelActionsMenuItems& osc::ModelActionsMenuItems::operator=(ModelActionsMenuItems&&) noexcept = default;
osc::ModelActionsMenuItems::~ModelActionsMenuItems() noexcept = default;

void osc::ModelActionsMenuItems::onDraw()
{
    m_Impl->onDraw();
}
