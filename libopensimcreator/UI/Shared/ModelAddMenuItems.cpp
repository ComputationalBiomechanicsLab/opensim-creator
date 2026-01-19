#include "ModelAddMenuItems.h"

#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/UI/ModelEditor/AddBodyPopup.h>
#include <libopensimcreator/UI/ModelEditor/AddComponentPopup.h>
#include <libopensimcreator/UI/ModelEditor/Select1PFPopup.h>
#include <libopensimcreator/UI/ModelEditor/SelectComponentPopup.h>
#include <libopensimcreator/UI/ModelEditor/SelectGeometryPopup.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>

#include <libopynsim/Documents/Model/IModelStatePair.h>
#include <libopynsim/ComponentRegistry/ComponentRegistry.h>
#include <libopynsim/ComponentRegistry/StaticComponentRegistries.h>
#include <libopynsim/Utils/OpenSimHelpers.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/widget.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utils/c_string_view.h>
#include <liboscar/utils/string_helpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Control/Controller.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathActuator.h>
#include <OpenSim/Simulation/Model/Probe.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>

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

    void setTargetParentComponent(const OpenSim::ComponentPath& path)
    {
        m_MaybeTargetParentComponent = path;
    }

    void onDraw()
    {
        ui::push_id(this);

        const bool disabled = m_Model->isReadonly();
        if (disabled) {
            ui::begin_disabled();
        }

        ui::set_next_item_width(ui::get_content_region_available().x());
        DrawSearchBar(m_SearchString);

        if (m_SearchString.empty()) {
            drawDefaultComponentList();
        }
        else {
            drawSearchResultsOrNoResults();
        }

        drawTargetComponentSpecializedAdders();

        if (disabled) {
            ui::end_disabled();
        }

        ui::pop_id();
    }

private:
    void drawTargetComponentSpecializedAdders()
    {
        const OpenSim::Component* component = FindComponent(m_Model->getModel(), m_MaybeTargetParentComponent);
        if (not component) {
            return;
        }

        if (const auto* joint = dynamic_cast<const OpenSim::Joint*>(component)) {
            ui::draw_separator();
            drawSpecializedContextualActions(*joint);
        }
        else if (const auto* hcf = dynamic_cast<const OpenSim::HuntCrossleyForce*>(component)) {
            ui::draw_separator();
            drawSpecializedContextualActions(*hcf);
        }
        else if (const auto* pa = dynamic_cast<const OpenSim::PathActuator*>(component)) {
            ui::draw_separator();
            drawSpecializedContextualActions(*pa);
        }
        else if (const auto* gp = dynamic_cast<const OpenSim::GeometryPath*>(component)) {
            ui::draw_separator();
            drawSpecializedContextualActions(*gp);
        }
        else if (const auto* pf = dynamic_cast<const OpenSim::PhysicalFrame*>(component)) {
            ui::draw_separator();
            drawSpecializedContextualActions(*pf);
        }
    }

    void drawSpecializedContextualActions(const OpenSim::Joint& joint)
    {
        if (ui::draw_menu_item("Parent Offset Frame", {}, nullptr, m_Model->canUpdModel())) {
            ActionAddParentOffsetFrameToJoint(*m_Model, joint.getAbsolutePath());
        }

        if (ui::draw_menu_item("Child Offset Frame", {}, nullptr, m_Model->canUpdModel())) {
            ActionAddChildOffsetFrameToJoint(*m_Model, joint.getAbsolutePath());
        }
    }

    void drawSpecializedContextualActions(const OpenSim::HuntCrossleyForce& hcf)
    {
        if (not parent()) {
            return;  // can't open the select contact geometry popup.
        }
        if (size(hcf.get_contact_parameters()) > 1) {
            return;  // cannot edit: has more than one HuntCrossleyForce::Parameter
        }

        if (ui::draw_menu_item("Associated Contact Geometry", {}, nullptr, m_Model->canUpdModel())) {
            const auto onSelection = [model = m_Model, path = hcf.getAbsolutePath()](const OpenSim::ComponentPath& geomPath)
            {
                ActionAssignContactGeometryToHCF(*model, path, geomPath);
            };
            const auto filter = [](const OpenSim::Component& c) -> bool
            {
                return dynamic_cast<const OpenSim::ContactGeometry*>(&c) != nullptr;
            };
            auto popup = std::make_unique<SelectComponentPopup>(parent(), "Select Contact Geometry", m_Model, onSelection, filter);
            App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered("Add Associated Contact Geometry", "Add an existing OpenSim::ContactGeometry in the model to this OpenSim::HuntCrossleyForce.\n\nCollisions are evaluated for all OpenSim::ContactGeometry attached to the OpenSim::HuntCrossleyForce. E.g. if you want an OpenSim::ContactSphere component to collide with an OpenSim::ContactHalfSpace component during a simulation then you should add both of those components to this force");
    }

    void drawSpecializedContextualActions(const OpenSim::PathActuator& pa)
    {
        if (not parent()) {
            return;  // required in order to open a popup
        }

        if (ui::draw_menu_item("Path Point", {}, nullptr, m_Model->canUpdModel())) {
            auto onSelection = [model = m_Model, path = pa.getAbsolutePath()](const OpenSim::ComponentPath& pfPath)
            {
                ActionAddPathPointToPathActuator(*model, path, pfPath);
            };
            auto popup = std::make_unique<Select1PFPopup>(parent(), "Select Physical Frame", m_Model, onSelection);
            App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered("Add Path Point", "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::GeometryPath");

        if (const auto* gp = dynamic_cast<const OpenSim::GeometryPath*>(&pa.getPath())) {
            if (ui::begin_menu("Path Wrap", m_Model->canUpdModel())) {
                drawPathWrapToggleMenuItems(*gp);
                ui::end_menu();
            }
        }
    }

    void drawSpecializedContextualActions(const OpenSim::GeometryPath& geometryPath)
    {
        if (ui::begin_menu("Path Wrap", m_Model->canUpdModel())) {
            drawPathWrapToggleMenuItems(geometryPath);
            ui::end_menu();
        }
        if (ui::draw_menu_item("Path Point", {}, nullptr, m_Model->canUpdModel())) {
            auto onSelection = [model = m_Model, path = geometryPath.getAbsolutePath()](const OpenSim::ComponentPath& pfPath)
            {
                ActionAddPathPointToGeometryPath(*model, path, pfPath);
            };
            auto popup = std::make_unique<Select1PFPopup>(parent(), "Select Physical Frame", m_Model, onSelection);
            App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered("Add Path Point", "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::PathActuator");
    }

    void drawSpecializedContextualActions(const OpenSim::PhysicalFrame& frame)
    {
        if (ui::draw_menu_item("Geometry", {}, nullptr, m_Model->canUpdModel() and parent() != nullptr)) {
            const std::function<void(std::unique_ptr<OpenSim::Geometry>)> callback = [
                model = m_Model,
                path = frame.getAbsolutePath(),
                pptr = parent()](auto geom)
            {
                ActionAttachGeometryToPhysicalFrame(*model, path, std::move(geom));
            };
            auto popup = std::make_unique<SelectGeometryPopup>(
                parent(),
                "select geometry to attach",
                App::resource_filepath("OpenSimCreator/geometry"),
                callback
            );
            App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered("Add Geometry", "Add geometry to this component. Geometry can be removed by selecting it in the navigator and pressing DELETE");

        if (ui::draw_menu_item("Offset Frame", {}, nullptr, m_Model->canUpdModel())) {
            ActionAddOffsetFrameToPhysicalFrame(*m_Model, frame.getAbsolutePath());
        }
        ui::draw_tooltip_if_item_hovered("Add Offset Frame", "Add an OpenSim::OffsetFrame as a child of this Component. Other components in the model can then connect to this OffsetFrame, rather than the base Component, so that it can connect at some offset that is relative to the parent Component");

        if (ui::begin_menu("Wrap Object", m_Model->canUpdModel())) {
            drawAddWrapObjectsToPhysicalFrameMenuItems(frame.getAbsolutePath());
            ui::end_menu();
        }
    }

    void drawPathWrapToggleMenuItems(const OpenSim::GeometryPath& gp)
    {
        const auto wraps = GetAllWrapObjectsReferencedBy(gp);
        for (const auto& wo : m_Model->getModel().getComponentList<OpenSim::WrapObject>()) {
            const bool enabled = cpp23::contains(wraps, &wo);

            ui::push_id(&wo);
            bool selected = enabled;
            if (ui::draw_menu_item(wo.getName(), {}, &selected, m_Model->canUpdModel())) {
                if (enabled) {
                    ActionRemoveWrapObjectFromGeometryPathWraps(*m_Model, gp, wo);
                }
                else {
                    ActionAddWrapObjectToGeometryPathWraps(*m_Model, gp, wo);
                }
            }
            ui::pop_id();
        }
    }

    void drawAddWrapObjectsToPhysicalFrameMenuItems(const OpenSim::ComponentPath& physicalFrameAbsPath)
    {
        // list each available `WrapObject` as something the user can add
        const auto& registry = GetComponentRegistry<OpenSim::WrapObject>();
        for (const auto& entry : registry) {
            ui::push_id(&entry);
            if (ui::draw_menu_item(entry.name(), {}, nullptr, m_Model->canUpdModel())) {
                ActionAddWrapObjectToPhysicalFrame(
                    *m_Model,
                    physicalFrameAbsPath,
                    entry.instantiate()
                );
            }
            ui::pop_id();
        }
    }

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
                    actionOpenComponentPopup(entry);
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
                    actionOpenComponentPopup(entry);
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

    void actionOpenComponentPopup(const ComponentRegistryEntryBase& entry)
    {
        if (not parent()) {
            return;  // Can't fire popup-opening event upwards.
        }

        std::stringstream label;
        label << "Add " << entry.name();
        const OpenSim::Component* target = FindComponent(*m_Model, m_MaybeTargetParentComponent);
        if (target) {
            label << " to " << target->getName();
        }

        auto popup = std::make_unique<AddComponentPopup>(
            &owner(),
            label.str(),
            m_Model,
            entry.instantiate(),
            m_MaybeTargetParentComponent
        );
        App::post_event<OpenPopupEvent>(owner(), std::move(popup));
    }

    std::shared_ptr<IModelStatePair> m_Model;
    std::string m_SearchString;
    OpenSim::ComponentPath m_MaybeTargetParentComponent;
};


osc::ModelAddMenuItems::ModelAddMenuItems(Widget* parent, std::shared_ptr<IModelStatePair> m) :
    Widget{std::make_unique<Impl>(*this, parent, std::move(m))}
{}

void osc::ModelAddMenuItems::setTargetParentComponent(const OpenSim::ComponentPath& path)
{
    private_data().setTargetParentComponent(path);
}

void osc::ModelAddMenuItems::impl_on_draw() { private_data().onDraw(); }
