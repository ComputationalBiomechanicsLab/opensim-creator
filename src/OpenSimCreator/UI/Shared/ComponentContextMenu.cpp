#include "ComponentContextMenu.h"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/Documents/Model/Environment.h>
#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/UI/Events/AddMusclePlotEvent.h>
#include <OpenSimCreator/UI/ModelEditor/ModelActionsMenuItems.h>
#include <OpenSimCreator/UI/ModelEditor/ReassignSocketPopup.h>
#include <OpenSimCreator/UI/ModelEditor/Select1PFPopup.h>
#include <OpenSimCreator/UI/ModelEditor/SelectComponentPopup.h>
#include <OpenSimCreator/UI/ModelEditor/SelectGeometryPopup.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathActuator.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/os.h>
#include <oscar/Platform/Widget.h>
#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/UI/Events/OpenNamedPanelEvent.h>
#include <oscar/UI/Events/OpenPopupEvent.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/LifetimedPtr.h>

#include <memory>
#include <sstream>
#include <string>
#include <utility>

using namespace osc;

// helpers
namespace
{
    // draw UI element that lets user change a model joint's type
    void DrawSelectionJointTypeSwitcher(
        IModelStatePair& model,
        const OpenSim::ComponentPath& jointPath)
    {
        const auto* joint = FindComponent<OpenSim::Joint>(model.getModel(), jointPath);
        if (not joint) {
            return;
        }

        const auto& registry = GetComponentRegistry<OpenSim::Joint>();

        std::optional<ptrdiff_t> selectedIdx;
        if (ui::begin_menu("Change Joint Type", model.canUpdModel())) {
            // look the Joint up in the type registry so we know where it should be in the ui::draw_combobox
            std::optional<size_t> maybeTypeIndex = IndexOf(registry, *joint);

            for (ptrdiff_t i = 0; i < std::ssize(registry); ++i) {
                bool selected = i == maybeTypeIndex;
                bool wasSelected = selected;

                if (ui::draw_menu_item(registry[i].name(), {}, &selected)) {
                    if (not wasSelected) {
                        selectedIdx = i;
                    }
                }
            }
            ui::end_menu();
        }

        if (selectedIdx and *selectedIdx < std::ssize(registry)) {
            // copy + fixup  a prototype of the user's selection
            ActionChangeJointTypeTo(
                model,
                jointPath,
                registry[*selectedIdx].instantiate()
            );
        }
    }

    // draw the `MenuItem`s for the "Add Wrap Object" menu
    void DrawAddWrapObjectsToPhysicalFrameMenuItems(
        IModelStatePair& modelState,
        const OpenSim::ComponentPath& physicalFrameAbsPath)
    {
        // list each available `WrapObject` as something the user can add
        const auto& registry = GetComponentRegistry<OpenSim::WrapObject>();
        for (const auto& entry : registry) {
            ui::push_id(&entry);
            if (ui::draw_menu_item(entry.name(), {}, nullptr, modelState.canUpdModel())) {
                ActionAddWrapObjectToPhysicalFrame(
                    modelState,
                    physicalFrameAbsPath,
                    entry.instantiate()
                );
            }
            ui::pop_id();
        }
    }

    // draw contextual actions (buttons, sliders) for a selected physical frame
    void DrawPhysicalFrameContextualActions(
        Widget& parent,
        const std::shared_ptr<IModelStatePair>& modelState,
        const OpenSim::ComponentPath& pfPath)
    {
        if (const auto* pf = FindComponent<OpenSim::PhysicalFrame>(modelState->getModel(), pfPath)) {
            DrawCalculateMenu(
                modelState->getModel(),
                modelState->getState(),
                *pf,
                CalculateMenuFlags::NoCalculatorIcon
            );
        }

        if (ui::begin_menu("Add", modelState->canUpdModel())) {
            if (ui::draw_menu_item("Geometry", {}, nullptr, modelState->canUpdModel())) {
                const std::function<void(std::unique_ptr<OpenSim::Geometry>)> callback = [modelState, pfPath](auto geom)
                {
                    ActionAttachGeometryToPhysicalFrame(*modelState, pfPath, std::move(geom));
                };
                auto popup = std::make_unique<SelectGeometryPopup>(
                    "select geometry to attach",
                    App::resource_filepath("geometry"),
                    callback
                );
                App::post_event<OpenPopupEvent>(parent, std::move(popup));
            }
            ui::draw_tooltip_if_item_hovered("Add Geometry", "Add geometry to this component. Geometry can be removed by selecting it in the navigator and pressing DELETE");

            if (ui::draw_menu_item("Offset Frame", {}, nullptr, modelState->canUpdModel())) {
                ActionAddOffsetFrameToPhysicalFrame(*modelState, pfPath);
            }
            ui::draw_tooltip_if_item_hovered("Add Offset Frame", "Add an OpenSim::OffsetFrame as a child of this Component. Other components in the model can then connect to this OffsetFrame, rather than the base Component, so that it can connect at some offset that is relative to the parent Component");

            if (ui::begin_menu("Wrap Object", modelState->canUpdModel())) {
                DrawAddWrapObjectsToPhysicalFrameMenuItems(*modelState, pfPath);
                ui::end_menu();
            }

            ui::end_menu();
        }
    }


    // draw contextual actions (buttons, sliders) for a selected joint
    void DrawJointContextualActions(
        IModelStatePair& modelState,
        const OpenSim::ComponentPath& jointPath)
    {
        DrawSelectionJointTypeSwitcher(modelState, jointPath);

        if (ui::draw_menu_item("Rezero Joint", {}, nullptr, CanRezeroJoint(modelState, jointPath))) {
            ActionRezeroJoint(modelState, jointPath);
        }
        ui::draw_tooltip_if_item_hovered("Re-zero the joint", "Given the joint's current geometry due to joint defaults, coordinate defaults, and any coordinate edits made in the coordinates panel, this will reorient the joint's parent (if it's an offset frame) to match the child's transformation. Afterwards, it will then resets all of the joints coordinates to zero. This effectively sets the 'zero point' of the joint (i.e. the geometry when all coordinates are zero) to match whatever the current geometry is.");

        if (ui::draw_menu_item("Add Parent Offset Frame", {}, nullptr, modelState.canUpdModel())) {
            ActionAddParentOffsetFrameToJoint(modelState, jointPath);
        }

        if (ui::draw_menu_item("Add Child Offset Frame", {}, nullptr, modelState.canUpdModel())) {
            ActionAddChildOffsetFrameToJoint(modelState, jointPath);
        }

        if (ui::draw_menu_item("Toggle Frame Visibility", {}, nullptr, modelState.canUpdModel())) {
            ActionToggleFrames(modelState);
        }
    }

    // draw contextual actions (buttons, sliders) for a selected joint
    void DrawHCFContextualActions(
        Widget& parent,
        const std::shared_ptr<IModelStatePair>& uim,
        const OpenSim::ComponentPath& hcfPath)
    {
        const auto* const hcf = FindComponent<OpenSim::HuntCrossleyForce>(uim->getModel(), hcfPath);
        if (not hcf) {
            return;
        }

        if (size(hcf->get_contact_parameters()) > 1) {
            return;  // cannot edit: has more than one HuntCrossleyForce::Parameter
        }

        if (ui::draw_menu_item("Add Contact Geometry", {}, nullptr, uim->canUpdModel())) {
            const auto onSelection = [uim, hcfPath](const OpenSim::ComponentPath& geomPath)
            {
                ActionAssignContactGeometryToHCF(*uim, hcfPath, geomPath);
            };
            const auto filter = [](const OpenSim::Component& c) -> bool
            {
                return dynamic_cast<const OpenSim::ContactGeometry*>(&c) != nullptr;
            };
            auto popup = std::make_unique<SelectComponentPopup>("Select Contact Geometry", uim, onSelection, filter);
            App::post_event<OpenPopupEvent>(parent, std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered("Add Contact Geometry", "Add OpenSim::ContactGeometry to this OpenSim::HuntCrossleyForce.\n\nCollisions are evaluated for all OpenSim::ContactGeometry attached to the OpenSim::HuntCrossleyForce. E.g. if you want an OpenSim::ContactSphere component to collide with an OpenSim::ContactHalfSpace component during a simulation then you should add both of those components to this force");
    }

    // draw contextual actions (buttons, sliders) for a selected path actuator
    void DrawPathActuatorContextualParams(
        Widget& parent,
        const std::shared_ptr<IModelStatePair>& modelState,
        const OpenSim::ComponentPath& paPath)
    {
        if (ui::draw_menu_item("Add Path Point", {}, nullptr, modelState->canUpdModel())) {
            auto onSelection = [modelState, paPath](const OpenSim::ComponentPath& pfPath) { ActionAddPathPointToPathActuator(*modelState, paPath, pfPath); };
            auto popup = std::make_unique<Select1PFPopup>("Select Physical Frame", modelState, onSelection);
            App::post_event<OpenPopupEvent>(parent, std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered("Add Path Point", "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::PathActuator");
    }

    void DrawModelContextualActions(IModelStatePair& modelState)
    {
        if (ui::draw_menu_item("Toggle Frames", {}, nullptr, modelState.canUpdModel())) {
            ActionToggleFrames(modelState);
        }
    }

    void DrawStationContextualActions(
        const IModelStatePair& modelState,
        const OpenSim::Station& station)
    {
        DrawCalculateMenu(
            modelState.getModel(),
            modelState.getState(),
            station,
            CalculateMenuFlags::NoCalculatorIcon
        );
    }

    void DrawPointContextualActions(
        const IModelStatePair& modelState,
        const OpenSim::Point& point)
    {
        DrawCalculateMenu(
            modelState.getModel(),
            modelState.getState(),
            point,
            CalculateMenuFlags::NoCalculatorIcon
        );
    }

    void DrawEllipsoidContextualActions(
        const IModelStatePair& modelState,
        const OpenSim::Ellipsoid& ellipsoid)
    {
        DrawCalculateMenu(
            modelState.getModel(),
            modelState.getState(),
            ellipsoid,
            CalculateMenuFlags::NoCalculatorIcon
        );
    }

    void DrawMeshContextualActions(
        IModelStatePair& modelState,
        const OpenSim::Mesh& mesh)
    {
        if (ui::begin_menu("Fit Analytic Geometry to This", modelState.canUpdModel())) {
            ui::draw_help_marker("Uses shape-fitting algorithms to fit analytic geometry to the points in the given mesh.\n\nThe 'htbad'-suffixed algorithms were adapted (potentially, with bugs - report them) from the MATLAB code in:\n\n        Bishop P., How to build a dinosaur..., doi:10.1017/pab.2020.46");

            if (ui::draw_menu_item("Sphere (htbad)", {}, nullptr, modelState.canUpdModel())) {
                ActionFitSphereToMesh(modelState, mesh);
            }

            if (ui::draw_menu_item("Ellipsoid (htbad)", {}, nullptr, modelState.canUpdModel())) {
                ActionFitEllipsoidToMesh(modelState, mesh);
            }

            if (ui::draw_menu_item("Plane (htbad)", {}, nullptr, modelState.canUpdModel())) {
                ActionFitPlaneToMesh(modelState, mesh);
            }

            ui::end_menu();
        }

        if (ui::begin_menu("Export")) {
            DrawMeshExportContextMenuContent(modelState, mesh);
            ui::end_menu();
        }
    }

    void DrawGeometryContextualActions(
        const IModelStatePair& modelState,
        const OpenSim::Geometry& geometry)
    {
        DrawCalculateMenu(
            modelState.getModel(),
            modelState.getState(),
            geometry,
            CalculateMenuFlags::NoCalculatorIcon
        );
    }

    void DrawPathWrapToggleMenuItems(
        IModelStatePair& modelState,
        const OpenSim::GeometryPath& gp)
    {
        const auto wraps = GetAllWrapObjectsReferencedBy(gp);
        for (const auto& wo : modelState.getModel().getComponentList<OpenSim::WrapObject>()) {
            const bool enabled = cpp23::contains(wraps, &wo);

            ui::push_id(&wo);
            bool selected = enabled;
            if (ui::draw_menu_item(wo.getName(), {}, &selected, modelState.canUpdModel())) {
                if (enabled) {
                    ActionRemoveWrapObjectFromGeometryPathWraps(modelState, gp, wo);
                }
                else {
                    ActionAddWrapObjectToGeometryPathWraps(modelState, gp, wo);
                }
            }
            ui::pop_id();
        }
    }

    void DrawGeometryPathContextualActions(
        IModelStatePair& modelState,
        const OpenSim::GeometryPath& geometryPath)
    {
        if (ui::begin_menu("Add", modelState.canUpdModel())) {
            if (ui::begin_menu("Path Wrap", modelState.canUpdModel())) {
                DrawPathWrapToggleMenuItems(modelState, geometryPath);
                ui::end_menu();
            }
            ui::end_menu();
        }
    }

    bool AnyDescendentInclusiveHasAppearanceProperty(const OpenSim::Component& component)
    {
        const OpenSim::Component* const c = FindFirstDescendentInclusive(
            component,
            [](const OpenSim::Component& desc) -> bool { return TryGetAppearance(desc) != nullptr; }
        );
        return c != nullptr;
    }
}

class osc::ComponentContextMenu::Impl final : public StandardPopup {
public:
    Impl(
        std::string_view popupName_,
        Widget& parent_,
        std::shared_ptr<IModelStatePair> model_,
        OpenSim::ComponentPath path_,
        ComponentContextMenuFlags flags_) :

        StandardPopup{popupName_, {10.0f, 10.0f}, ui::WindowFlag::NoMove},
        m_Parent{parent_.weak_ref()},
        m_Model{std::move(model_)},
        m_Path{std::move(path_)},
        m_Flags{flags_}
    {
        set_modal(false);
        OSC_ASSERT(m_Model != nullptr);
    }

private:
    void impl_draw_content() final
    {
        const OpenSim::Component* c = FindComponent(m_Model->getModel(), m_Path);
        if (not c) {

            // draw context menu content that's shown when nothing was right-clicked
            DrawNothingRightClickedContextMenuHeader();
            DrawContextMenuSeparator();
            if (ui::begin_menu("Add", m_Model->canUpdModel())) {
                m_ModelActionsMenuBar.onDraw();
                ui::end_menu();
            }

            // draw a display menu to match the display menu that appears when right-clicking
            // something, but this display menu only contains the functionality to show everything
            // in the model
            //
            // it's handy when users have selectively hidden this-or-that, or have hidden everything
            // in the model (#422)
            if (ui::begin_menu("Display", m_Model->canUpdModel())) {
                if (ui::draw_menu_item("Show All")) {
                    ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetRootComponentPath(), true);
                }
                ui::draw_tooltip_if_item_hovered("Show All", "Sets the visiblity of all components within the model to 'visible', handy for undoing selective hiding etc.");
                ui::end_menu();
            }
            return;
        }

        DrawRightClickedComponentContextMenuHeader(*c);
        DrawContextMenuSeparator();

        DrawWatchOutputMenu(*c, [this](const OpenSim::AbstractOutput& output, std::optional<ComponentOutputSubfield> subfield)
        {
            std::shared_ptr<Environment> environment = m_Model->tryUpdEnvironment();
            if (subfield) {
                environment->addUserOutputExtractor(OutputExtractor{ComponentOutputExtractor{output, *subfield}});
            }
            else {
                environment->addUserOutputExtractor(OutputExtractor{ComponentOutputExtractor{output}});
            }

            // when the user asks to watch an output, make sure the "Output Watches" panel is
            // open, so that they can immediately see the side-effect of watching an output (#567)
            App::post_event<OpenNamedPanelEvent>(*m_Parent, "Output Watches");
        });

        if (ui::begin_menu("Display", m_Model->canUpdModel())) {
            const bool shouldDisable = m_Model->isReadonly() or not AnyDescendentInclusiveHasAppearanceProperty(*c);

            if (ui::draw_menu_item("Show", {}, nullptr, shouldDisable)) {
                ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetAbsolutePath(*c), true);
            }

            if (ui::draw_menu_item("Show Only This", {}, nullptr, shouldDisable)) {
                ActionShowOnlyComponentAndAllChildren(*m_Model, GetAbsolutePath(*c));
            }

            if (ui::draw_menu_item("Hide", {}, nullptr, shouldDisable)) {
                ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetAbsolutePath(*c), false);
            }

            // add a seperator between probably commonly-used, simple, diplay toggles and the more
            // advanced ones
            ui::draw_separator();

            // redundantly put a "Show All" option here, also, so that the user doesn't have
            // to "know" that they need to right-click in the middle of nowhere or on the
            // model
            if (ui::draw_menu_item("Show All", {}, nullptr, shouldDisable)) {
                ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetRootComponentPath(), true);
            }

            {
                std::stringstream ss;
                ss << "Show All '" << c->getConcreteClassName() << "' Components";
                const std::string label = std::move(ss).str();
                if (ui::draw_menu_item(label, {}, nullptr, m_Model->canUpdModel())) {
                    ActionSetComponentAndAllChildrenWithGivenConcreteClassNameIsVisibleTo(
                        *m_Model,
                        GetAbsolutePath(m_Model->getModel()),
                        c->getConcreteClassName(),
                        true
                    );
                }
            }

            {
                std::stringstream ss;
                ss << "Hide All '" << c->getConcreteClassName() << "' Components";
                const std::string label = std::move(ss).str();
                if (ui::draw_menu_item(label, {}, nullptr, m_Model->canUpdModel())) {
                    ActionSetComponentAndAllChildrenWithGivenConcreteClassNameIsVisibleTo(
                        *m_Model,
                        GetAbsolutePath(m_Model->getModel()),
                        c->getConcreteClassName(),
                        false
                    );
                }
            }
            ui::end_menu();
        }

        if (ui::draw_menu_item("Copy Absolute Path to Clipboard")) {
            set_clipboard_text(GetAbsolutePathString(*c));
        }
        ui::draw_tooltip_if_item_hovered("Copy Component Absolute Path", "Copy the absolute path to this component to your clipboard.\n\n(This is handy if you are separately using absolute component paths to (e.g.) manipulate the model in a script or something)");

        drawSocketMenu(*c);

        if (dynamic_cast<const OpenSim::Model*>(c)) {
            DrawModelContextualActions(*m_Model);
        }
        else if (dynamic_cast<const OpenSim::PhysicalFrame*>(c)) {
            DrawPhysicalFrameContextualActions(*m_Parent, m_Model, m_Path);
        }
        else if (dynamic_cast<const OpenSim::Joint*>(c)) {
            DrawJointContextualActions(*m_Model, m_Path);
        }
        else if (dynamic_cast<const OpenSim::HuntCrossleyForce*>(c)) {
            DrawHCFContextualActions(*m_Parent, m_Model, m_Path);
        }
        else if (const auto* musclePtr = dynamic_cast<const OpenSim::Muscle*>(c)) {
            drawAddMusclePlotMenu(*musclePtr);
            DrawPathActuatorContextualParams(*m_Parent, m_Model, m_Path);  // a muscle is a path actuator
        }
        else if (dynamic_cast<const OpenSim::PathActuator*>(c)) {
            DrawPathActuatorContextualParams(*m_Parent, m_Model, m_Path);
        }
        else if (const auto* stationPtr = dynamic_cast<const OpenSim::Station*>(c)) {
            DrawStationContextualActions(*m_Model, *stationPtr);
        }
        else if (const auto* pointPtr = dynamic_cast<const OpenSim::Point*>(c)) {
            DrawPointContextualActions(*m_Model, *pointPtr);
        }
        else if (const auto* ellipsoidPtr = dynamic_cast<const OpenSim::Ellipsoid*>(c)) {
            DrawEllipsoidContextualActions(*m_Model, *ellipsoidPtr);
        }
        else if (const auto* meshPtr = dynamic_cast<const OpenSim::Mesh*>(c)) {
            DrawMeshContextualActions(*m_Model, *meshPtr);
        }
        else if (const auto* geomPtr = dynamic_cast<const OpenSim::Geometry*>(c)) {
            DrawGeometryContextualActions(*m_Model, *geomPtr);
        }
        else if (const auto* geomPathPtr = dynamic_cast<const OpenSim::GeometryPath*>(c)) {
            DrawGeometryPathContextualActions(*m_Model, *geomPathPtr);
        }
    }

    void drawSocketMenu(const OpenSim::Component& c)
    {
        if (ui::begin_menu("Sockets", m_Model->canUpdModel())) {
            std::vector<std::string> socketNames = GetSocketNames(c);

            if (not socketNames.empty()) {
                ui::push_style_var(ui::StyleVar::CellPadding, {0.5f*ui::get_text_line_height(), 0.5f*ui::get_text_line_height()});

                if (ui::begin_table("sockets table", 3, {ui::TableFlag::SizingStretchProp, ui::TableFlag::BordersInner, ui::TableFlag::PadOuterX})) {
                    ui::table_setup_column("Socket Name");
                    ui::table_setup_column("Connectee");
                    ui::table_setup_column("Actions");

                    ui::table_headers_row();

                    int id = 0;
                    for (const std::string& socketName : socketNames) {
                        const OpenSim::AbstractSocket& socket = c.getSocket(socketName);

                        int column = 0;
                        ui::push_id(id++);
                        ui::table_next_row();

                        ui::table_set_column_index(column++);
                        ui::draw_text_disabled(socketName);

                        ui::table_set_column_index(column++);
                        if (ui::draw_small_button(socket.getConnecteeAsObject().getName())) {
                            m_Model->setSelected(dynamic_cast<const OpenSim::Component*>(&socket.getConnecteeAsObject()));
                            request_close();
                        }
                        if (const auto* connectee = dynamic_cast<const OpenSim::Component*>(&socket.getConnecteeAsObject());
                            connectee && ui::is_item_hovered()) {

                            DrawComponentHoverTooltip(*connectee);
                        }

                        ui::table_set_column_index(column++);
                        if (ui::draw_small_button("change")) {
                            auto popup = std::make_unique<ReassignSocketPopup>(
                                "Reassign " + socket.getName(),
                                m_Model,
                                GetAbsolutePathString(c),
                                socketName
                            );
                            App::post_event<OpenPopupEvent>(*m_Parent, std::move(popup));
                        }

                        ui::pop_id();
                    }

                    ui::end_table();
                }
                ui::pop_style_var();
            }
            else {
                ui::draw_text_disabled("%s has no sockets", c.getName().c_str());
            }

            ui::end_menu();
        }
    }

    void drawAddMusclePlotMenu(const OpenSim::Muscle& m)
    {
        if (m_Flags & ComponentContextMenuFlag::NoPlotVsCoordinate) {
            return;
        }
        if (ui::begin_menu("Plot vs. Coordinate")) {
            for (const OpenSim::Coordinate& c : m_Model->getModel().getComponentList<OpenSim::Coordinate>()) {
                if (ui::draw_menu_item(c.getName())) {
                    App::post_event<AddMusclePlotEvent>(*m_Parent, c, m);
                }
            }

            ui::end_menu();
        }
    }

    LifetimedPtr<Widget> m_Parent;
    std::shared_ptr<IModelStatePair> m_Model;
    OpenSim::ComponentPath m_Path;
    ModelActionsMenuItems m_ModelActionsMenuBar{*m_Parent, m_Model};
    ComponentContextMenuFlags m_Flags;
};


osc::ComponentContextMenu::ComponentContextMenu(
    std::string_view popupName_,
    Widget& parent_,
    std::shared_ptr<IModelStatePair> model_,
    const OpenSim::ComponentPath& path_,
    ComponentContextMenuFlags flags_) :

    m_Impl{std::make_unique<Impl>(popupName_, parent_, std::move(model_), path_, flags_)}
{}
osc::ComponentContextMenu::ComponentContextMenu(ComponentContextMenu&&) noexcept = default;
osc::ComponentContextMenu& osc::ComponentContextMenu::operator=(ComponentContextMenu&&) noexcept = default;
osc::ComponentContextMenu::~ComponentContextMenu() noexcept = default;

bool osc::ComponentContextMenu::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::ComponentContextMenu::impl_open()
{
    m_Impl->open();
}

void osc::ComponentContextMenu::impl_close()
{
    m_Impl->close();
}

bool osc::ComponentContextMenu::impl_begin_popup()
{
    return m_Impl->begin_popup();
}

void osc::ComponentContextMenu::impl_on_draw()
{
    m_Impl->on_draw();
}

void osc::ComponentContextMenu::impl_end_popup()
{
    m_Impl->end_popup();
}
