#include "ComponentContextMenu.h"

#include <libopensimcreator/ComponentRegistry/ComponentRegistry.h>
#include <libopensimcreator/ComponentRegistry/StaticComponentRegistries.h>
#include <libopensimcreator/Documents/Model/Environment.h>
#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Documents/OutputExtractors/ComponentOutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractor.h>
#include <libopensimcreator/UI/Events/AddMusclePlotEvent.h>
#include <libopensimcreator/UI/ModelEditor/ReassignSocketPopup.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/UI/Shared/ModelAddMenuItems.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/platform/app.h>
#include <liboscar/platform/os.h>
#include <liboscar/platform/widget.h>
#include <liboscar/ui/events/open_named_panel_event.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/icon_cache.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <liboscar/utils/Assertions.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>

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

    // draw contextual actions (buttons, sliders) for a selected physical frame
    void DrawPhysicalFrameContextualActions(
        Widget&,
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

    void DrawMarkerContextualActions(
        IModelStatePair& modelState,
        const OpenSim::Marker& marker)
    {
        DrawCalculateMenu(
            modelState.getModel(),
            modelState.getState(),
            marker,
            CalculateMenuFlags::NoCalculatorIcon
        );

        // Show a specialized `Move To` menu that lets users move the marker to
        // the model's `MarkerSet`, which can be required for backwards compatibility
        // with OpenSim GUI (#1102).
        if (ui::begin_menu("Move To", not modelState.isReadonly())) {

            // Only enable this option if the marker isn't already part of the model's `MarkerSet`
            // (otherwise, we assume it's remove-able from its current owner).
            const OpenSim::Component* owner = GetOwner<OpenSim::MarkerSet>(marker);
            bool disabled = (owner != nullptr) and GetOwner<OpenSim::Model>(*owner) == &modelState.getModel();

            if (ui::draw_menu_item("/markerset", std::nullopt, nullptr, not disabled)) {
                ActionMoveMarkerToModelMarkerSet(modelState, marker);
            }
            if (disabled and ui::is_item_hovered(ui::HoveredFlag::AllowWhenDisabled)) {
                ui::draw_tooltip_body_only("This marker is already part of /markerset");
            }

            ui::end_menu();
        }
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
        if (ui::begin_menu("Fit Analytic Geometry", modelState.canUpdModel())) {
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

    bool AnyDescendentInclusiveHasAppearanceProperty(const OpenSim::Component& component)
    {
        const OpenSim::Component* const c = FindFirstDescendentInclusive(
            component,
            [](const OpenSim::Component& desc) -> bool { return TryGetAppearance(desc) != nullptr; }
        );
        return c != nullptr;
    }
}

class osc::ComponentContextMenu::Impl final : public PopupPrivate {
public:
    explicit Impl(
        ComponentContextMenu& owner,
        Widget* parent_,
        std::string_view popupName_,
        std::shared_ptr<IModelStatePair> model_,
        OpenSim::ComponentPath path_,
        ComponentContextMenuFlags flags_) :

        PopupPrivate{owner, parent_, popupName_, {10.0f, 10.0f}, ui::PanelFlag::NoMove},
        m_Model{std::move(model_)},
        m_Path{std::move(path_)},
        m_Flags{flags_}
    {
        set_modal(false);
        OSC_ASSERT(m_Model != nullptr);
    }

    void draw_content()
    {
        const OpenSim::Component* c = FindComponent(m_Model->getModel(), m_Path);
        if (not c) {
            // draw context menu content that's shown when nothing was right-clicked
            DrawNothingRightClickedContextMenuHeader();
            DrawContextMenuSeparator();
            if (ui::begin_menu("Add", m_Model->canUpdModel())) {
                m_ModelAddMenuItems.setTargetParentComponent({});  // i.e. the target parent component should default to the model
                m_ModelAddMenuItems.on_draw();
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
                ui::draw_vertical_spacer(0.5f);
                ui::draw_text_disabled("Model Visual Preferences");
                ui::draw_separator();
                DrawAllDecorationToggleButtons(*m_Model, *m_IconCache);
                ui::end_menu();
            }
            if (ui::begin_menu("Watch Output", false)) {
                ui::end_menu();
            }
            if (ui::begin_menu("Sockets", false)) {
                ui::end_menu();
            }
            if (ui::begin_menu("Copy", false)) {
                ui::end_menu();
            }
            return;
        }

        DrawRightClickedComponentContextMenuHeader(*c);
        DrawContextMenuSeparator();

        if (ui::begin_menu("Add", m_Model->canUpdModel())) {
            m_ModelAddMenuItems.setTargetParentComponent(m_Path);
            m_ModelAddMenuItems.on_draw();
            ui::end_menu();
        }

        if (ui::begin_menu("Display", m_Model->canUpdModel())) {
            drawDisplayMenuContent(*c);
            ui::end_menu();
        }

        DrawWatchOutputMenu(*c, [this](const OutputExtractor& outputExtractor)
        {
            m_Model->tryUpdEnvironment()->addUserOutputExtractor(outputExtractor);

            // when the user asks to watch an output, make sure the "Output Watches" panel is
            // open, so that they can immediately see the side-effect of watching an output (#567)
            App::post_event<OpenNamedPanelEvent>(owner(), "Output Watches");
        });

        drawSocketMenu(*c);

        if (ui::begin_menu("Copy")) {
            if (ui::draw_menu_item("Name to Clipboard")) {
                set_clipboard_text(c->getName());
            }
            if (ui::draw_menu_item("Absolute Path to Clipboard")) {
                set_clipboard_text(GetAbsolutePathString(*c));
            }
            ui::draw_tooltip_if_item_hovered("Copy Component Absolute Path", "Copy the absolute path to this component to your clipboard.\n\n(This is handy if you are separately using absolute component paths to (e.g.) manipulate the model in a script or something)");
            if (ui::draw_menu_item("Concrete Class Name to Clipboard")) {
                set_clipboard_text(c->getConcreteClassName());
            }
            if (ui::draw_menu_item("Component XML to Clipboard")) {
                set_clipboard_text(WriteObjectXMLToString(*c));
            }
            ui::end_menu();
        }

        if (dynamic_cast<const OpenSim::PhysicalFrame*>(c)) {
            ui::draw_separator();
            DrawPhysicalFrameContextualActions(owner(), m_Model, m_Path);
        }
        else if (dynamic_cast<const OpenSim::Joint*>(c)) {
            ui::draw_separator();
            DrawJointContextualActions(*m_Model, m_Path);
        }
        else if (const auto* musclePtr = dynamic_cast<const OpenSim::Muscle*>(c)) {
            ui::draw_separator();
            drawPlotVsCoordinateMenu(*musclePtr);
        }
        else if (const auto* markerPtr = dynamic_cast<const OpenSim::Marker*>(c)) {
            ui::draw_separator();
            DrawMarkerContextualActions(*m_Model, *markerPtr);
        }
        else if (const auto* stationPtr = dynamic_cast<const OpenSim::Station*>(c)) {
            ui::draw_separator();
            DrawStationContextualActions(*m_Model, *stationPtr);
        }
        else if (const auto* pointPtr = dynamic_cast<const OpenSim::Point*>(c)) {
            ui::draw_separator();
            DrawPointContextualActions(*m_Model, *pointPtr);
        }
        else if (const auto* ellipsoidPtr = dynamic_cast<const OpenSim::Ellipsoid*>(c)) {
            ui::draw_separator();
            DrawEllipsoidContextualActions(*m_Model, *ellipsoidPtr);
        }
        else if (const auto* meshPtr = dynamic_cast<const OpenSim::Mesh*>(c)) {
            ui::draw_separator();
            DrawMeshContextualActions(*m_Model, *meshPtr);
        }
        else if (const auto* geomPtr = dynamic_cast<const OpenSim::Geometry*>(c)) {
            ui::draw_separator();
            DrawGeometryContextualActions(*m_Model, *geomPtr);
        }
    }

private:
    void drawDisplayMenuContent(const OpenSim::Component& c)
    {
        const bool isEnabled = m_Model->canUpdModel() and AnyDescendentInclusiveHasAppearanceProperty(c);

        // togges that are specific to this components (+ its descendants)

        if (ui::draw_menu_item("Show", {}, nullptr, isEnabled)) {
            ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetAbsolutePath(c), true);
        }

        if (ui::draw_menu_item("Show Only This", {}, nullptr, isEnabled)) {
            ActionShowOnlyComponentAndAllChildren(*m_Model, GetAbsolutePath(c));
        }

        if (ui::draw_menu_item("Hide", {}, nullptr, isEnabled)) {
            ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetAbsolutePath(c), false);
        }

        // add a seperator between probably commonly-used, simple, diplay toggles and the more
        // advanced ones
        ui::draw_separator();

        // redundantly put a "Show All" option here, also, so that the user doesn't have
        // to "know" that they need to right-click in the middle of nowhere or on the
        // model
        if (ui::draw_menu_item("Show All", {}, nullptr, isEnabled)) {
            ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetRootComponentPath(), true);
        }

        {
            std::stringstream ss;
            ss << "Show All '" << c.getConcreteClassName() << "' Components";
            const std::string label = std::move(ss).str();
            if (ui::draw_menu_item(label, {}, nullptr, isEnabled)) {
                ActionSetComponentAndAllChildrenWithGivenConcreteClassNameIsVisibleTo(
                    *m_Model,
                    GetAbsolutePath(m_Model->getModel()),
                    c.getConcreteClassName(),
                    true
                );
            }
        }

        {
            std::stringstream ss;
            ss << "Hide All '" << c.getConcreteClassName() << "' Components";
            const std::string label = std::move(ss).str();
            if (ui::draw_menu_item(label, {}, nullptr, isEnabled)) {
                ActionSetComponentAndAllChildrenWithGivenConcreteClassNameIsVisibleTo(
                    *m_Model,
                    GetAbsolutePath(m_Model->getModel()),
                    c.getConcreteClassName(),
                    false
                );
            }
        }

        ui::draw_vertical_spacer(0.5f);
        ui::draw_text_disabled("Model Visual Preferences");
        ui::draw_separator();
        DrawAllDecorationToggleButtons(*m_Model, *m_IconCache);
    }

    void drawSocketMenu(const OpenSim::Component& c)
    {
        if (ui::begin_menu("Sockets", m_Model->canUpdModel())) {

            ui::draw_text_centered("Outbound Sockets");
            ui::draw_separator();
            drawOutboundSocketsInfo(c);

            ui::start_new_line();

            ui::draw_text_centered("Inbound Connections");
            ui::draw_separator();
            drawInboundConnectionsInfo(c);

            ui::end_menu();
        }
    }

    void drawOutboundSocketsInfo(const OpenSim::Component& c)
    {
        if (c.getNumSockets() != 0) {
            drawOutboundSocketsTable(c);
        } else {
            ui::draw_dummy({256.0f, 0.0f});
            ui::draw_text_disabled_and_centered(c.getName() + " has no outbound sockets.");
        }
    }

    void drawOutboundSocketsTable(const OpenSim::Component& c)
    {
        const std::vector<std::string> socketNames = GetSocketNames(c);
        ui::push_style_var(ui::StyleVar::CellPadding, ui::get_text_line_height_in_current_panel() * Vector2{0.5f});

        if (ui::begin_table("outbound sockets table", 4, {ui::TableFlag::SizingStretchProp, ui::TableFlag::BordersInner, ui::TableFlag::PadOuterX})) {
            ui::table_setup_column("Socket Name");
            ui::table_setup_column("Connectee Type");
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
                ui::draw_text(socket.getConnecteeTypeName());

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
                        &owner(),
                        "Reassign " + socket.getName(),
                        m_Model,
                        GetAbsolutePathString(c),
                        socketName
                    );
                    App::post_event<OpenPopupEvent>(owner(), std::move(popup));
                }

                ui::pop_id();
            }

            ui::end_table();
        }
        ui::pop_style_var();
    }

    void drawInboundConnectionsInfo(const OpenSim::Component& c)
    {
        const auto filter = m_ShouldFilterInboundConnections ?
            [](const OpenSim::Component& c) { return ShouldShowInUI(c) and dynamic_cast<const OpenSim::FrameGeometry*>(&c) == nullptr; } :
            [](const OpenSim::Component&)   { return true; };

        auto els = ForEachInboundConnection(&m_Model->getModel(), &c, filter);
        auto it = els.begin();
        const auto end = els.end();

        if (it != end) {
            drawInboundConnectionsTable(it, end);
        }
        else {
            ui::draw_dummy({256.0f, 0.0f});
            ui::draw_text_disabled_and_centered(c.getName() + " has no inbound sockets.");
        }

        ui::indent();
        ui::draw_checkbox("Hide Junk", &m_ShouldFilterInboundConnections);
        ui::unindent();
    }

    void drawInboundConnectionsTable(auto& it, std::default_sentinel_t end)
    {
        ui::push_style_var(ui::StyleVar::CellPadding, ui::get_text_line_height_in_current_panel() * Vector2{0.5f});
        const ui::TableFlags flags = {
            ui::TableFlag::SizingStretchProp,
            ui::TableFlag::BordersInner,
            ui::TableFlag::PadOuterX,
            ui::TableFlag::ScrollY
        };
        const Vector2 dimensions = Vector2{0.0f, 10.0f*ui::get_text_line_height_with_spacing_in_current_panel()};
        if (ui::begin_table("inbound connections table", 3, flags, dimensions)) {
            ui::table_setup_column("Source Component");
            ui::table_setup_column("Socket Name");
            ui::table_setup_column("Actions");

            ui::table_headers_row();

            // draw data rows
            int id = 0;
            for (; it != end; ++it) {
                const ComponentConnectionView view{*it};

                int column = 0;
                ui::push_id(id++);
                ui::table_next_row();

                // column: Source Component
                ui::table_set_column_index(column++);
                if (ui::draw_small_button(view.source().getName())) {
                    m_Model->setSelected(dynamic_cast<const OpenSim::Component*>(&view.source()));
                    request_close();
                }
                if (ui::is_item_hovered()) {
                    DrawComponentHoverTooltip(view.source());
                }

                // column: Socket Name
                ui::table_set_column_index(column++);
                ui::draw_text_disabled(view.socketName());

                // column: actions
                ui::table_set_column_index(column++);
                if (ui::draw_small_button("change")) {
                    auto popup = std::make_unique<ReassignSocketPopup>(
                        &owner(),
                        "Reassign " + view.socketName(),
                        m_Model,
                        GetAbsolutePathString(view.source()),
                        view.socketName()
                    );
                    App::post_event<OpenPopupEvent>(owner(), std::move(popup));
                }

                ui::pop_id();
            }

            ui::end_table();
        }
        ui::pop_style_var();
    }

    void drawPlotVsCoordinateMenu(const OpenSim::Muscle& m)
    {
        if (m_Flags & ComponentContextMenuFlag::NoPlotVsCoordinate) {
            return;
        }
        if (ui::begin_menu("Plot vs. Coordinate")) {
            for (const OpenSim::Coordinate& c : m_Model->getModel().getComponentList<OpenSim::Coordinate>()) {
                if (ui::draw_menu_item(c.getName())) {
                    App::post_event<AddMusclePlotEvent>(owner(), c, m);
                }
            }

            ui::end_menu();
        }
    }

    std::shared_ptr<IModelStatePair> m_Model;
    OpenSim::ComponentPath m_Path;
    ModelAddMenuItems m_ModelAddMenuItems{&owner(), m_Model};
    ComponentContextMenuFlags m_Flags;
    bool m_ShouldFilterInboundConnections = true;
    std::shared_ptr<IconCache> m_IconCache = App::singleton<IconCache>(
        App::resource_loader().with_prefix("OpenSimCreator/icons/"),
        ui::get_font_base_size()/128.0f,
        App::get().highest_device_pixel_ratio()
    );
};


osc::ComponentContextMenu::ComponentContextMenu(
    Widget* parent_,
    std::string_view popupName_,
    std::shared_ptr<IModelStatePair> model_,
    const OpenSim::ComponentPath& path_,
    ComponentContextMenuFlags flags_) :

    Popup{std::make_unique<Impl>(*this, parent_, popupName_, std::move(model_), path_, flags_)}
{}
void osc::ComponentContextMenu::impl_draw_content() { private_data().draw_content(); }
