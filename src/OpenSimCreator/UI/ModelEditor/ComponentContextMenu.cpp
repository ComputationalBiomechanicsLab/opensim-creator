#include "ComponentContextMenu.h"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/UI/ModelEditor/ModelActionsMenuItems.h>
#include <OpenSimCreator/UI/ModelEditor/ReassignSocketPopup.h>
#include <OpenSimCreator/UI/ModelEditor/Select1PFPopup.h>
#include <OpenSimCreator/UI/ModelEditor/SelectComponentPopup.h>
#include <OpenSimCreator/UI/ModelEditor/SelectGeometryPopup.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
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
#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/ParentPtr.h>

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
        UndoableModelStatePair& uim,
        OpenSim::ComponentPath const& jointPath)
    {
        auto const* joint = FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
        if (!joint)
        {
            return;
        }

        auto const& registry = GetComponentRegistry<OpenSim::Joint>();

        std::optional<ptrdiff_t> selectedIdx;
        if (ui::begin_menu("Change Joint Type"))
        {
            // look the Joint up in the type registry so we know where it should be in the ui::draw_combobox
            std::optional<size_t> maybeTypeIndex = IndexOf(registry, *joint);

            for (ptrdiff_t i = 0; i < std::ssize(registry); ++i)
            {
                bool selected = i == maybeTypeIndex;
                bool wasSelected = selected;

                if (ui::draw_menu_item(registry[i].name(), {}, &selected))
                {
                    if (!wasSelected)
                    {
                        selectedIdx = i;
                    }
                }
            }
            ui::end_menu();
        }

        if (selectedIdx && *selectedIdx < std::ssize(registry))
        {
            // copy + fixup  a prototype of the user's selection
            ActionChangeJointTypeTo(
                uim,
                jointPath,
                registry[*selectedIdx].instantiate()
            );
        }
    }

    // draw the `MenuItem`s for the "Add Wrap Object" menu
    void DrawAddWrapObjectsToPhysicalFrameMenuItems(
        IEditorAPI*,
        std::shared_ptr<UndoableModelStatePair> const& uim,
        OpenSim::ComponentPath const& physicalFrameAbsPath)
    {
        // list each available `WrapObject` as something the user can add
        auto const& registry = GetComponentRegistry<OpenSim::WrapObject>();
        for (auto const& entry : registry) {
            ui::push_id(&entry);
            if (ui::draw_menu_item(entry.name())) {
                ActionAddWrapObjectToPhysicalFrame(
                    *uim,
                    physicalFrameAbsPath,
                    entry.instantiate()
                );
            }
            ui::pop_id();
        }
    }

    // draw contextual actions (buttons, sliders) for a selected physical frame
    void DrawPhysicalFrameContextualActions(
        IEditorAPI* editorAPI,
        std::shared_ptr<UndoableModelStatePair> const& uim,
        OpenSim::ComponentPath const& pfPath)
    {
        if (auto const* pf = FindComponent<OpenSim::PhysicalFrame>(uim->getModel(), pfPath))
        {
            DrawCalculateMenu(
                uim->getModel(),
                uim->getState(),
                *pf,
                CalculateMenuFlags::NoCalculatorIcon
            );
        }

        if (ui::begin_menu("Add")) {
            if (ui::draw_menu_item("Geometry")) {
                std::function<void(std::unique_ptr<OpenSim::Geometry>)> const callback = [uim, pfPath](auto geom)
                {
                    ActionAttachGeometryToPhysicalFrame(*uim, pfPath, std::move(geom));
                };
                auto p = std::make_unique<SelectGeometryPopup>(
                    "select geometry to attach",
                    App::resource_filepath("geometry"),
                    callback
                );
                p->open();
                editorAPI->pushPopup(std::move(p));
            }
            ui::draw_tooltip_if_item_hovered("Add Geometry", "Add geometry to this component. Geometry can be removed by selecting it in the navigator and pressing DELETE");

            if (ui::draw_menu_item("Offset Frame")) {
                ActionAddOffsetFrameToPhysicalFrame(*uim, pfPath);
            }
            ui::draw_tooltip_if_item_hovered("Add Offset Frame", "Add an OpenSim::OffsetFrame as a child of this Component. Other components in the model can then connect to this OffsetFrame, rather than the base Component, so that it can connect at some offset that is relative to the parent Component");

            if (ui::begin_menu("Wrap Object")) {
                DrawAddWrapObjectsToPhysicalFrameMenuItems(editorAPI, uim, pfPath);
                ui::end_menu();
            }

            ui::end_menu();
        }
    }


    // draw contextual actions (buttons, sliders) for a selected joint
    void DrawJointContextualActions(
        UndoableModelStatePair& uim,
        OpenSim::ComponentPath const& jointPath)
    {
        DrawSelectionJointTypeSwitcher(uim, jointPath);

        if (CanRezeroJoint(uim, jointPath))
        {
            if (ui::draw_menu_item("Rezero Joint"))
            {
                ActionRezeroJoint(uim, jointPath);
            }
            ui::draw_tooltip_if_item_hovered("Re-zero the joint", "Given the joint's current geometry due to joint defaults, coordinate defaults, and any coordinate edits made in the coordinates panel, this will reorient the joint's parent (if it's an offset frame) to match the child's transformation. Afterwards, it will then resets all of the joints coordinates to zero. This effectively sets the 'zero point' of the joint (i.e. the geometry when all coordinates are zero) to match whatever the current geometry is.");
        }

        if (ui::draw_menu_item("Add Parent Offset Frame"))
        {
            ActionAddParentOffsetFrameToJoint(uim, jointPath);
        }

        if (ui::draw_menu_item("Add Child Offset Frame"))
        {
            ActionAddChildOffsetFrameToJoint(uim, jointPath);
        }

        if (ui::draw_menu_item("Toggle Frame Visibility"))
        {
            ActionToggleFrames(uim);
        }
    }

    // draw contextual actions (buttons, sliders) for a selected joint
    void DrawHCFContextualActions(
        IEditorAPI* api,
        std::shared_ptr<UndoableModelStatePair> const& uim,
        OpenSim::ComponentPath const& hcfPath)
    {
        auto const* const hcf = FindComponent<OpenSim::HuntCrossleyForce>(uim->getModel(), hcfPath);
        if (!hcf)
        {
            return;
        }

        if (size(hcf->get_contact_parameters()) > 1)
        {
            return;  // cannot edit: has more than one HuntCrossleyForce::Parameter
        }

        if (ui::draw_menu_item("Add Contact Geometry"))
        {
            auto onSelection = [uim, hcfPath](OpenSim::ComponentPath const& geomPath)
            {
                ActionAssignContactGeometryToHCF(*uim, hcfPath, geomPath);
            };
            auto filter = [](OpenSim::Component const& c) -> bool
            {
                return dynamic_cast<OpenSim::ContactGeometry const*>(&c) != nullptr;
            };
            auto popup = std::make_unique<SelectComponentPopup>("Select Contact Geometry", uim, onSelection, filter);
            popup->open();
            api->pushPopup(std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered("Add Contact Geometry", "Add OpenSim::ContactGeometry to this OpenSim::HuntCrossleyForce.\n\nCollisions are evaluated for all OpenSim::ContactGeometry attached to the OpenSim::HuntCrossleyForce. E.g. if you want an OpenSim::ContactSphere component to collide with an OpenSim::ContactHalfSpace component during a simulation then you should add both of those components to this force");
    }

    // draw contextual actions (buttons, sliders) for a selected path actuator
    void DrawPathActuatorContextualParams(
        IEditorAPI* api,
        std::shared_ptr<UndoableModelStatePair> const& uim,
        OpenSim::ComponentPath const& paPath)
    {
        if (ui::draw_menu_item("Add Path Point"))
        {
            auto onSelection = [uim, paPath](OpenSim::ComponentPath const& pfPath) { ActionAddPathPointToPathActuator(*uim, paPath, pfPath); };
            auto popup = std::make_unique<Select1PFPopup>("Select Physical Frame", uim, onSelection);
            popup->open();
            api->pushPopup(std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered("Add Path Point", "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::PathActuator");
    }

    void DrawModelContextualActions(UndoableModelStatePair& uim)
    {
        if (ui::draw_menu_item("Toggle Frames"))
        {
            ActionToggleFrames(uim);
        }
    }

    void DrawStationContextualActions(
        UndoableModelStatePair& uim,
        OpenSim::Station const& station)
    {
        DrawCalculateMenu(
            uim.getModel(),
            uim.getState(),
            station,
            CalculateMenuFlags::NoCalculatorIcon
        );
    }

    void DrawPointContextualActions(
        UndoableModelStatePair& uim,
        OpenSim::Point const& point)
    {
        DrawCalculateMenu(
            uim.getModel(),
            uim.getState(),
            point,
            CalculateMenuFlags::NoCalculatorIcon
        );
    }

    void DrawEllipsoidContextualActions(
        UndoableModelStatePair& uim,
        OpenSim::Ellipsoid const& ellipsoid)
    {
        DrawCalculateMenu(
            uim.getModel(),
            uim.getState(),
            ellipsoid,
            CalculateMenuFlags::NoCalculatorIcon
        );
    }

    void DrawMeshContextualActions(
        UndoableModelStatePair& uim,
        OpenSim::Mesh const& mesh)
    {
        if (ui::begin_menu("Fit Analytic Geometry to This"))
        {
            ui::draw_help_marker("Uses shape-fitting algorithms to fit analytic geometry to the points in the given mesh.\n\nThe 'htbad'-suffixed algorithms were adapted (potentially, with bugs - report them) from the MATLAB code in:\n\n        Bishop P., How to build a dinosaur..., doi:10.1017/pab.2020.46");

            if (ui::draw_menu_item("Sphere (htbad)"))
            {
                ActionFitSphereToMesh(uim, mesh);
            }

            if (ui::draw_menu_item("Ellipsoid (htbad)"))
            {
                ActionFitEllipsoidToMesh(uim, mesh);
            }

            if (ui::draw_menu_item("Plane (htbad)"))
            {
                ActionFitPlaneToMesh(uim, mesh);
            }

            ui::end_menu();
        }

        if (ui::begin_menu("Export"))
        {
            DrawMeshExportContextMenuContent(uim, mesh);
            ui::end_menu();
        }
    }

    void DrawGeometryContextualActions(
        UndoableModelStatePair& uim,
        OpenSim::Geometry const& geometry)
    {
        DrawCalculateMenu(
            uim.getModel(),
            uim.getState(),
            geometry,
            CalculateMenuFlags::NoCalculatorIcon
        );
    }

    void DrawPathWrapToggleMenuItems(
        UndoableModelStatePair& uim,
        OpenSim::GeometryPath const& gp)
    {
        auto const wraps = GetAllWrapObjectsReferencedBy(gp);
        for (auto const& wo : uim.getModel().getComponentList<OpenSim::WrapObject>()) {
            bool const enabled = cpp23::contains(wraps, &wo);

            ui::push_id(&wo);
            bool selected = enabled;
            if (ui::draw_menu_item(wo.getName(), {}, &selected)) {
                if (enabled) {
                    ActionRemoveWrapObjectFromGeometryPathWraps(uim, gp, wo);
                }
                else {
                    ActionAddWrapObjectToGeometryPathWraps(uim, gp, wo);
                }
            }
            ui::pop_id();
        }
    }

    void DrawGeometryPathContextualActions(
        UndoableModelStatePair& uim,
        OpenSim::GeometryPath const& geometryPath)
    {
        if (ui::begin_menu("Add")) {
            if (ui::begin_menu("Path Wrap")) {
                DrawPathWrapToggleMenuItems(uim, geometryPath);
                ui::end_menu();
            }
            ui::end_menu();
        }
    }

    bool AnyDescendentInclusiveHasAppearanceProperty(OpenSim::Component const& component)
    {
        OpenSim::Component const* const c = FindFirstDescendentInclusive(
            component,
            [](OpenSim::Component const& desc) -> bool { return TryGetAppearance(desc) != nullptr; }
        );
        return c != nullptr;
    }
}

class osc::ComponentContextMenu::Impl final : public StandardPopup {
public:
    Impl(
        std::string_view popupName_,
        ParentPtr<IMainUIStateAPI> const& mainUIStateAPI_,
        IEditorAPI* editorAPI_,
        std::shared_ptr<UndoableModelStatePair> model_,
        OpenSim::ComponentPath path_) :

        StandardPopup{popupName_, {10.0f, 10.0f}, ImGuiWindowFlags_NoMove},
        m_MainUIStateAPI{mainUIStateAPI_},
        m_EditorAPI{editorAPI_},
        m_Model{std::move(model_)},
        m_Path{std::move(path_)}
    {
        set_modal(false);
        OSC_ASSERT(m_Model != nullptr);
    }

private:
    void impl_draw_content() final
    {
        OpenSim::Component const* c = FindComponent(m_Model->getModel(), m_Path);
        if (!c)
        {
            // draw context menu content that's shown when nothing was right-clicked
            DrawNothingRightClickedContextMenuHeader();
            DrawContextMenuSeparator();
            if (ui::begin_menu("Add"))
            {
                m_ModelActionsMenuBar.onDraw();
                ui::end_menu();
            }

            // draw a display menu to match the display menu that appears when right-clicking
            // something, but this display menu only contains the functionality to show everything
            // in the model
            //
            // it's handy when users have selectively hidden this-or-that, or have hidden everything
            // in the model (#422)
            if (ui::begin_menu("Display"))
            {
                if (ui::draw_menu_item("Show All"))
                {
                    ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetRootComponentPath(), true);
                }
                ui::draw_tooltip_if_item_hovered("Show All", "Sets the visiblity of all components within the model to 'visible', handy for undoing selective hiding etc.");
                ui::end_menu();
            }
            return;
        }

        DrawRightClickedComponentContextMenuHeader(*c);
        DrawContextMenuSeparator();

        DrawWatchOutputMenu(*c, [this](OpenSim::AbstractOutput const& output, std::optional<ComponentOutputSubfield> subfield)
        {
            if (subfield) {
                m_MainUIStateAPI->addUserOutputExtractor(OutputExtractor{ComponentOutputExtractor{output, *subfield}});
            }
            else {
                m_MainUIStateAPI->addUserOutputExtractor(OutputExtractor{ComponentOutputExtractor{output}});
            }

            // when the user asks to watch an output, make sure the "Output Watches" panel is
            // open, so that they can immediately see the side-effect of watching an output (#567)
            m_EditorAPI->getPanelManager()->set_toggleable_panel_activated("Output Watches", true);
        });

        if (ui::begin_menu("Display"))
        {
            bool const shouldDisable = !AnyDescendentInclusiveHasAppearanceProperty(*c);

            {
                if (shouldDisable)
                {
                    ui::begin_disabled();
                }
                if (ui::draw_menu_item("Show"))
                {
                    ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetAbsolutePath(*c), true);
                }
                if (shouldDisable)
                {
                    ui::end_disabled();
                }
            }

            {
                if (shouldDisable)
                {
                    ui::begin_disabled();
                }
                if (ui::draw_menu_item("Show Only This"))
                {
                    ActionShowOnlyComponentAndAllChildren(*m_Model, GetAbsolutePath(*c));
                }
                if (shouldDisable)
                {
                    ui::end_disabled();
                }
            }

            {
                if (shouldDisable)
                {
                    ui::begin_disabled();
                }
                if (ui::draw_menu_item("Hide"))
                {
                    ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetAbsolutePath(*c), false);
                }
                if (shouldDisable)
                {
                    ui::end_disabled();
                }
            }

            // add a seperator between probably commonly-used, simple, diplay toggles and the more
            // advanced ones
            ui::draw_separator();

            // redundantly put a "Show All" option here, also, so that the user doesn't have
            // to "know" that they need to right-click in the middle of nowhere or on the
            // model
            if (ui::draw_menu_item("Show All"))
            {
                ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetRootComponentPath(), true);
            }

            {
                std::stringstream ss;
                ss << "Show All '" << c->getConcreteClassName() << "' Components";
                std::string const label = std::move(ss).str();
                if (ui::draw_menu_item(label))
                {
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
                std::string const label = std::move(ss).str();
                if (ui::draw_menu_item(label))
                {
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

        if (ui::draw_menu_item("Copy Absolute Path to Clipboard"))
        {
            std::string const path = GetAbsolutePathString(*c);
            set_clipboard_text(path);
        }
        ui::draw_tooltip_if_item_hovered("Copy Component Absolute Path", "Copy the absolute path to this component to your clipboard.\n\n(This is handy if you are separately using absolute component paths to (e.g.) manipulate the model in a script or something)");

        drawSocketMenu(*c);

        if (dynamic_cast<OpenSim::Model const*>(c))
        {
            DrawModelContextualActions(*m_Model);
        }
        else if (dynamic_cast<OpenSim::PhysicalFrame const*>(c))
        {
            DrawPhysicalFrameContextualActions(m_EditorAPI, m_Model, m_Path);
        }
        else if (dynamic_cast<OpenSim::Joint const*>(c))
        {
            DrawJointContextualActions(*m_Model, m_Path);
        }
        else if (dynamic_cast<OpenSim::HuntCrossleyForce const*>(c))
        {
            DrawHCFContextualActions(m_EditorAPI, m_Model, m_Path);
        }
        else if (auto const* musclePtr = dynamic_cast<OpenSim::Muscle const*>(c))
        {
            drawAddMusclePlotMenu(*musclePtr);
            DrawPathActuatorContextualParams(m_EditorAPI, m_Model, m_Path);  // a muscle is a path actuator
        }
        else if (dynamic_cast<OpenSim::PathActuator const*>(c))
        {
            DrawPathActuatorContextualParams(m_EditorAPI, m_Model, m_Path);
        }
        else if (auto const* stationPtr = dynamic_cast<OpenSim::Station const*>(c))
        {
            DrawStationContextualActions(*m_Model, *stationPtr);
        }
        else if (auto const* pointPtr = dynamic_cast<OpenSim::Point const*>(c))
        {
            DrawPointContextualActions(*m_Model, *pointPtr);
        }
        else if (auto const* ellipsoidPtr = dynamic_cast<OpenSim::Ellipsoid const*>(c))
        {
            DrawEllipsoidContextualActions(*m_Model, *ellipsoidPtr);
        }
        else if (auto const* meshPtr = dynamic_cast<OpenSim::Mesh const*>(c))
        {
            DrawMeshContextualActions(*m_Model, *meshPtr);
        }
        else if (auto const* geomPtr = dynamic_cast<OpenSim::Geometry const*>(c))
        {
            DrawGeometryContextualActions(*m_Model, *geomPtr);
        }
        else if (auto const* geomPathPtr = dynamic_cast<OpenSim::GeometryPath const*>(c))
        {
            DrawGeometryPathContextualActions(*m_Model, *geomPathPtr);
        }
    }

    void drawSocketMenu(OpenSim::Component const& c)
    {
        if (ui::begin_menu("Sockets"))
        {
            std::vector<std::string> socketNames = GetSocketNames(c);

            if (!socketNames.empty())
            {
                ui::push_style_var(ImGuiStyleVar_CellPadding, {0.5f*ui::get_text_line_height(), 0.5f*ui::get_text_line_height()});
                if (ui::begin_table("sockets table", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInner | ImGuiTableFlags_PadOuterX))
                {
                    ui::table_setup_column("Socket Name");
                    ui::table_setup_column("Connectee");
                    ui::table_setup_column("Actions");

                    ui::table_headers_row();

                    int id = 0;
                    for (std::string const& socketName : socketNames)
                    {
                        OpenSim::AbstractSocket const& socket = c.getSocket(socketName);

                        int column = 0;
                        ui::push_id(id++);
                        ui::table_next_row();

                        ui::table_set_column_index(column++);
                        ui::draw_text_disabled(socketName);

                        ui::table_set_column_index(column++);
                        if (ui::draw_small_button(socket.getConnecteeAsObject().getName()))
                        {
                            m_Model->setSelected(dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject()));
                            request_close();
                        }
                        if (auto const* connectee = dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject());
                            connectee && ui::is_item_hovered())
                        {
                            DrawComponentHoverTooltip(*connectee);
                        }

                        ui::table_set_column_index(column++);
                        if (ui::draw_small_button("change"))
                        {
                            auto popup = std::make_unique<ReassignSocketPopup>(
                                "Reassign " + socket.getName(),
                                m_Model,
                                GetAbsolutePathString(c),
                                socketName
                            );
                            popup->open();
                            m_EditorAPI->pushPopup(std::move(popup));
                        }

                        ui::pop_id();
                    }

                    ui::end_table();
                }
                ui::pop_style_var();
            }
            else
            {
                ui::draw_text_disabled("%s has no sockets", c.getName().c_str());
            }

            ui::end_menu();
        }
    }

    void drawAddMusclePlotMenu(OpenSim::Muscle const& m)
    {
        if (ui::begin_menu("Plot vs. Coordinate"))
        {
            for (OpenSim::Coordinate const& c : m_Model->getModel().getComponentList<OpenSim::Coordinate>())
            {
                if (ui::draw_menu_item(c.getName()))
                {
                    m_EditorAPI->addMusclePlot(c, m);
                }
            }

            ui::end_menu();
        }
    }

    ParentPtr<IMainUIStateAPI> m_MainUIStateAPI;
    IEditorAPI* m_EditorAPI = nullptr;
    std::shared_ptr<UndoableModelStatePair> m_Model;
    OpenSim::ComponentPath m_Path;
    ModelActionsMenuItems m_ModelActionsMenuBar{m_EditorAPI, m_Model};
};


// public API (PIMPL)

osc::ComponentContextMenu::ComponentContextMenu(
    std::string_view popupName_,
    ParentPtr<IMainUIStateAPI> const& mainUIStateAPI_,
    IEditorAPI* editorAPI_,
    std::shared_ptr<UndoableModelStatePair> model_,
    OpenSim::ComponentPath const& path_) :

    m_Impl{std::make_unique<Impl>(popupName_, mainUIStateAPI_, editorAPI_, std::move(model_), path_)}
{
}

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
