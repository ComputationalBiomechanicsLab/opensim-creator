#include "ModelActionsMenuItems.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
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
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

#include <concepts>
#include <sstream>
#include <utility>

class osc::ModelActionsMenuItems::Impl final {
public:

    Impl(
        IEditorAPI* api,
        std::shared_ptr<IModelStatePair> uum_) :

        m_EditorAPI{api},
        m_Model{std::move(uum_)}
    {}

    void onDraw()
    {
        ui::push_id(this);

        // action: add body
        {
            // draw button
            if (ui::draw_menu_item("Body")) {
                auto popup = std::make_unique<AddBodyPopup>("add body", m_EditorAPI, m_Model);
                popup->open();
                m_EditorAPI->pushPopup(std::move(popup));
            }

            // draw tooltip (if hovered)
            if (ui::is_item_hovered(ui::HoveredFlag::DelayNormal)) {
                ui::draw_tooltip(
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

        ui::pop_id();
    }

private:

    void renderButton(const ComponentRegistryBase& registry)
    {
        if (ui::begin_menu(registry.name())) {
            for (const auto& entry : registry) {
                if (ui::draw_menu_item(entry.name())) {
                    auto popup = std::make_unique<AddComponentPopup>(
                        "Add " + registry.name(),
                        m_EditorAPI,
                        m_Model,
                        entry.instantiate()
                    );
                    popup->open();
                    m_EditorAPI->pushPopup(std::move(popup));
                }

                if (ui::is_item_hovered(ui::HoveredFlag::DelayNormal)) {
                    ui::draw_tooltip(entry.name(), entry.description());
                }
            }

            ui::end_menu();
        }

        if (ui::is_item_hovered(ui::HoveredFlag::DelayNormal)) {
            ui::draw_tooltip(registry.name(), registry.description());
        }
    }

    IEditorAPI* m_EditorAPI;
    std::shared_ptr<IModelStatePair> m_Model;
};


osc::ModelActionsMenuItems::ModelActionsMenuItems(IEditorAPI* api, std::shared_ptr<IModelStatePair> m) :
    m_Impl{std::make_unique<Impl>(api, std::move(m))}
{}
osc::ModelActionsMenuItems::ModelActionsMenuItems(ModelActionsMenuItems&&) noexcept = default;
osc::ModelActionsMenuItems& osc::ModelActionsMenuItems::operator=(ModelActionsMenuItems&&) noexcept = default;
osc::ModelActionsMenuItems::~ModelActionsMenuItems() noexcept = default;

void osc::ModelActionsMenuItems::onDraw() { m_Impl->onDraw(); }
