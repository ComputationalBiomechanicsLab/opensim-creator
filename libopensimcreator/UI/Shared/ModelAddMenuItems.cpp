#include "ModelAddMenuItems.h"

#include <libopensimcreator/ComponentRegistry/ComponentRegistry.h>
#include <libopensimcreator/ComponentRegistry/StaticComponentRegistries.h>
#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/UI/ModelEditor/AddBodyPopup.h>
#include <libopensimcreator/UI/ModelEditor/AddComponentPopup.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Platform/App.h>
#include <liboscar/Platform/Widget.h>
#include <liboscar/Platform/WidgetPrivate.h>
#include <liboscar/UI/Events/OpenPopupEvent.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/LifetimedPtr.h>
#include <liboscar/Utils/StringHelpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Control/Controller.h>
#include <OpenSim/Simulation/Model/Actuator.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Probe.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>

#include <concepts>
#include <sstream>
#include <utility>

class osc::ModelAddMenuItems::Impl final : public WidgetPrivate {
public:

    explicit Impl(
        Widget& owner,
        Widget* parent,
        std::shared_ptr<IModelStatePair> uum_) :

        WidgetPrivate{owner, parent},
        m_Model{std::move(uum_)}
    {}

    void onDraw()
    {
        ui::push_id(this);

        const bool disabled = m_Model->isReadonly();
        if (disabled) {
            ui::begin_disabled();
        }

        ui::set_next_item_width(ui::get_content_region_available().x);
        DrawSearchBar(m_SearchString);

        if (m_SearchString.empty()) {
            drawDefaultComponentList();
        }
        else {
            drawSearchResultsOrNoResults();
        }

        if (disabled) {
            ui::end_disabled();
        }

        ui::pop_id();
    }

private:
    void drawDefaultComponentList()
    {
        // action: add body
        {
            // draw button
            if (ui::draw_menu_item("Body", {}, nullptr, m_Model->canUpdModel())) {
                if (parent()) {
                    auto popup = std::make_unique<AddBodyPopup>(&owner(), "add body", m_Model);
                    App::post_event<OpenPopupEvent>(owner(), std::move(popup));
                }
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
    }

    void drawSearchResultsOrNoResults()
    {
        bool searchResultFount = false;
        for (const auto& entry : GetAllRegisteredComponents()) {
            if (contains_case_insensitive(entry.name(), m_SearchString)) {
                if (ui::draw_menu_item(entry.name())) {
                    if (parent()) {
                        auto popup = std::make_unique<AddComponentPopup>(
                            &owner(),
                            "Add Component",
                            m_Model,
                            entry.instantiate()
                        );
                        App::post_event<OpenPopupEvent>(owner(), std::move(popup));
                    }
                }
                searchResultFount = true;
            }
        }
        if (not searchResultFount) {
            ui::draw_text_disabled_and_centered("no results ");
            ui::same_line();
            if (ui::draw_small_button("clear search")) {
                m_SearchString.clear();
            }
        }
    }

    void renderButton(const ComponentRegistryBase& registry)
    {
        if (ui::begin_menu(registry.name(), m_Model->canUpdModel())) {
            for (const auto& entry : registry) {
                if (ui::draw_menu_item(entry.name())) {
                    if (parent()) {
                        auto popup = std::make_unique<AddComponentPopup>(
                            &owner(),
                            "Add " + registry.name(),
                            m_Model,
                            entry.instantiate()
                        );
                        App::post_event<OpenPopupEvent>(owner(), std::move(popup));
                    }
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

    std::shared_ptr<IModelStatePair> m_Model;
    std::string m_SearchString;
};


osc::ModelAddMenuItems::ModelAddMenuItems(Widget* parent, std::shared_ptr<IModelStatePair> m) :
    Widget{std::make_unique<Impl>(*this, parent, std::move(m))}
{}

void osc::ModelAddMenuItems::impl_on_draw() { private_data().onDraw(); }
