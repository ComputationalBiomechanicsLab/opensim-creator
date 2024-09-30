#include "ModelActionsMenuItems.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/UI/ModelEditor/AddBodyPopup.h>
#include <OpenSimCreator/UI/ModelEditor/AddComponentPopup.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Control/Controller.h>
#include <OpenSim/Simulation/Model/Actuator.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Probe.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Widget.h>
#include <oscar/UI/Events/OpenPopupEvent.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/LifetimedPtr.h>

#include <concepts>
#include <sstream>
#include <utility>

class osc::ModelActionsMenuItems::Impl final {
public:

    Impl(
        Widget& parent,
        std::shared_ptr<IModelStatePair> uum_) :

        m_Parent{parent.weak_ref()},
        m_Model{std::move(uum_)}
    {}

    void onDraw()
    {
        ui::push_id(this);

        const bool disabled = m_Model->isReadonly();
        if (disabled) {
            ui::begin_disabled();
        }

        // action: add body
        {
            // draw button
            if (ui::draw_menu_item("Body", {}, nullptr, m_Model->canUpdModel())) {
                auto popup = std::make_unique<AddBodyPopup>("add body", *m_Parent, m_Model);
                App::post_event<OpenPopupEvent>(*m_Parent, std::move(popup));
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

        if (disabled) {
            ui::end_disabled();
        }

        ui::pop_id();
    }

private:

    void renderButton(const ComponentRegistryBase& registry)
    {
        if (ui::begin_menu(registry.name(), m_Model->canUpdModel())) {
            for (const auto& entry : registry) {
                if (ui::draw_menu_item(entry.name())) {
                    auto popup = std::make_unique<AddComponentPopup>(
                        "Add " + registry.name(),
                        *m_Parent,
                        m_Model,
                        entry.instantiate()
                    );
                    App::post_event<OpenPopupEvent>(*m_Parent, std::move(popup));
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

    LifetimedPtr<Widget> m_Parent;
    std::shared_ptr<IModelStatePair> m_Model;
};


osc::ModelActionsMenuItems::ModelActionsMenuItems(Widget& parent, std::shared_ptr<IModelStatePair> m) :
    m_Impl{std::make_unique<Impl>(parent, std::move(m))}
{}
osc::ModelActionsMenuItems::ModelActionsMenuItems(ModelActionsMenuItems&&) noexcept = default;
osc::ModelActionsMenuItems& osc::ModelActionsMenuItems::operator=(ModelActionsMenuItems&&) noexcept = default;
osc::ModelActionsMenuItems::~ModelActionsMenuItems() noexcept = default;

void osc::ModelActionsMenuItems::onDraw() { m_Impl->onDraw(); }
