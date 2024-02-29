#include "ComponentContextMenu.h"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
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
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Widgets/StandardPopup.h>
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
        if (ImGui::BeginMenu("Change Joint Type"))
        {
            // look the Joint up in the type registry so we know where it should be in the ImGui::Combo
            std::optional<size_t> maybeTypeIndex = IndexOf(registry, *joint);

            for (ptrdiff_t i = 0; i < std::ssize(registry); ++i)
            {
                bool selected = i == maybeTypeIndex;
                bool wasSelected = selected;

                if (ImGui::MenuItem(registry[i].name().c_str(), nullptr, &selected))
                {
                    if (!wasSelected)
                    {
                        selectedIdx = i;
                    }
                }
            }
            ImGui::EndMenu();
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

        if (ImGui::MenuItem("Add Geometry"))
        {
            std::function<void(std::unique_ptr<OpenSim::Geometry>)> const callback = [uim, pfPath](auto geom)
            {
                ActionAttachGeometryToPhysicalFrame(*uim, pfPath, std::move(geom));
            };
            auto p = std::make_unique<SelectGeometryPopup>(
                "select geometry to attach",
                App::resourceFilepath("geometry"),
                callback
            );
            p->open();
            editorAPI->pushPopup(std::move(p));
        }
        DrawTooltipIfItemHovered("Add Geometry", "Add geometry to this component. Geometry can be removed by selecting it in the navigator and pressing DELETE");

        if (ImGui::MenuItem("Add Offset Frame"))
        {
            ActionAddOffsetFrameToPhysicalFrame(*uim, pfPath);
        }
        DrawTooltipIfItemHovered("Add Offset Frame", "Add an OpenSim::OffsetFrame as a child of this Component. Other components in the model can then connect to this OffsetFrame, rather than the base Component, so that it can connect at some offset that is relative to the parent Component");
    }


    // draw contextual actions (buttons, sliders) for a selected joint
    void DrawJointContextualActions(
        UndoableModelStatePair& uim,
        OpenSim::ComponentPath const& jointPath)
    {
        DrawSelectionJointTypeSwitcher(uim, jointPath);

        if (CanRezeroJoint(uim, jointPath))
        {
            if (ImGui::MenuItem("Rezero Joint"))
            {
                ActionRezeroJoint(uim, jointPath);
            }
            DrawTooltipIfItemHovered("Re-zero the joint", "Given the joint's current geometry due to joint defaults, coordinate defaults, and any coordinate edits made in the coordinates panel, this will reorient the joint's parent (if it's an offset frame) to match the child's transformation. Afterwards, it will then resets all of the joints coordinates to zero. This effectively sets the 'zero point' of the joint (i.e. the geometry when all coordinates are zero) to match whatever the current geometry is.");
        }

        if (ImGui::MenuItem("Add Parent Offset Frame"))
        {
            ActionAddParentOffsetFrameToJoint(uim, jointPath);
        }

        if (ImGui::MenuItem("Add Child Offset Frame"))
        {
            ActionAddChildOffsetFrameToJoint(uim, jointPath);
        }

        if (ImGui::MenuItem("Toggle Frame Visibility"))
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

        if (ImGui::MenuItem("Add Contact Geometry"))
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
        DrawTooltipIfItemHovered("Add Contact Geometry", "Add OpenSim::ContactGeometry to this OpenSim::HuntCrossleyForce.\n\nCollisions are evaluated for all OpenSim::ContactGeometry attached to the OpenSim::HuntCrossleyForce. E.g. if you want an OpenSim::ContactSphere component to collide with an OpenSim::ContactHalfSpace component during a simulation then you should add both of those components to this force");
    }

    // draw contextual actions (buttons, sliders) for a selected path actuator
    void DrawPathActuatorContextualParams(
        IEditorAPI* api,
        std::shared_ptr<UndoableModelStatePair> const& uim,
        OpenSim::ComponentPath const& paPath)
    {
        if (ImGui::MenuItem("Add Path Point"))
        {
            auto onSelection = [uim, paPath](OpenSim::ComponentPath const& pfPath) { ActionAddPathPointToPathActuator(*uim, paPath, pfPath); };
            auto popup = std::make_unique<Select1PFPopup>("Select Physical Frame", uim, onSelection);
            popup->open();
            api->pushPopup(std::move(popup));
        }
        DrawTooltipIfItemHovered("Add Path Point", "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::PathActuator");
    }

    void DrawModelContextualActions(UndoableModelStatePair& uim)
    {
        if (ImGui::MenuItem("Toggle Frames"))
        {
            ActionToggleFrames(uim);
        }
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
        if (ImGui::BeginMenu("Fit Analytic Geometry to This"))
        {
            DrawHelpMarker("Uses shape-fitting algorithms to fit analytic geometry to the points in the given mesh.\n\nThe 'htbad'-suffixed algorithms were adapted (potentially, with bugs - report them) from the MATLAB code in:\n\n        Bishop P., How to build a dinosaur..., doi:10.1017/pab.2020.46");

            if (ImGui::MenuItem("Sphere (htbad)"))
            {
                ActionFitSphereToMesh(uim, mesh);
            }

            if (ImGui::MenuItem("Ellipsoid (htbad)"))
            {
                ActionFitEllipsoidToMesh(uim, mesh);
            }

            if (ImGui::MenuItem("Plane (htbad)"))
            {
                ActionFitPlaneToMesh(uim, mesh);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Export"))
        {
            DrawMeshExportContextMenuContent(uim, mesh);
            ImGui::EndMenu();
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
        setModal(false);
        OSC_ASSERT(m_Model != nullptr);
    }

private:
    void implDrawContent() final
    {
        OpenSim::Component const* c = FindComponent(m_Model->getModel(), m_Path);
        if (!c)
        {
            // draw context menu content that's shown when nothing was right-clicked
            DrawNothingRightClickedContextMenuHeader();
            DrawContextMenuSeparator();
            if (ImGui::BeginMenu("Add"))
            {
                m_ModelActionsMenuBar.onDraw();
                ImGui::EndMenu();
            }

            // draw a display menu to match the display menu that appears when right-clicking
            // something, but this display menu only contains the functionality to show everything
            // in the model
            //
            // it's handy when users have selectively hidden this-or-that, or have hidden everything
            // in the model (#422)
            if (ImGui::BeginMenu("Display"))
            {
                if (ImGui::MenuItem("Show All"))
                {
                    ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetRootComponentPath(), true);
                }
                DrawTooltipIfItemHovered("Show All", "Sets the visiblity of all components within the model to 'visible', handy for undoing selective hiding etc.");
                ImGui::EndMenu();
            }
            return;
        }

        DrawRightClickedComponentContextMenuHeader(*c);
        DrawContextMenuSeparator();

        //DrawSelectOwnerMenu(*m_Model, *c);
        if (DrawWatchOutputMenu(*m_MainUIStateAPI, *c))
        {
            // when the user asks to watch an output, make sure the "Output Watches" panel is
            // open, so that they can immediately see the side-effect of watching an output (#567)
            m_EditorAPI->getPanelManager()->setToggleablePanelActivated("Output Watches", true);
        }

        if (ImGui::BeginMenu("Display"))
        {
            bool const shouldDisable = !AnyDescendentInclusiveHasAppearanceProperty(*c);

            {
                if (shouldDisable)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::MenuItem("Show"))
                {
                    ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetAbsolutePath(*c), true);
                }
                if (shouldDisable)
                {
                    ImGui::EndDisabled();
                }
            }

            {
                if (shouldDisable)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::MenuItem("Show Only This"))
                {
                    ActionShowOnlyComponentAndAllChildren(*m_Model, GetAbsolutePath(*c));
                }
                if (shouldDisable)
                {
                    ImGui::EndDisabled();
                }
            }

            {
                if (shouldDisable)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::MenuItem("Hide"))
                {
                    ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetAbsolutePath(*c), false);
                }
                if (shouldDisable)
                {
                    ImGui::EndDisabled();
                }
            }

            // add a seperator between probably commonly-used, simple, diplay toggles and the more
            // advanced ones
            ImGui::Separator();

            // redundantly put a "Show All" option here, also, so that the user doesn't have
            // to "know" that they need to right-click in the middle of nowhere or on the
            // model
            if (ImGui::MenuItem("Show All"))
            {
                ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, GetRootComponentPath(), true);
            }

            {
                std::stringstream ss;
                ss << "Show All '" << c->getConcreteClassName() << "' Components";
                std::string const label = std::move(ss).str();
                if (ImGui::MenuItem(label.c_str()))
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
                if (ImGui::MenuItem(label.c_str()))
                {
                    ActionSetComponentAndAllChildrenWithGivenConcreteClassNameIsVisibleTo(
                        *m_Model,
                        GetAbsolutePath(m_Model->getModel()),
                        c->getConcreteClassName(),
                        false
                    );
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Copy Absolute Path to Clipboard"))
        {
            std::string const path = GetAbsolutePathString(*c);
            SetClipboardText(path.c_str());
        }
        DrawTooltipIfItemHovered("Copy Component Absolute Path", "Copy the absolute path to this component to your clipboard.\n\n(This is handy if you are separately using absolute component paths to (e.g.) manipulate the model in a script or something)");

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
    }

    void drawSocketMenu(OpenSim::Component const& c)
    {
        if (ImGui::BeginMenu("Sockets"))
        {
            std::vector<std::string> socketNames = GetSocketNames(c);

            if (!socketNames.empty())
            {
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {0.5f*ImGui::GetTextLineHeight(), 0.5f*ImGui::GetTextLineHeight()});
                if (ImGui::BeginTable("sockets table", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInner | ImGuiTableFlags_PadOuterX))
                {
                    ImGui::TableSetupColumn("Socket Name");
                    ImGui::TableSetupColumn("Connectee");
                    ImGui::TableSetupColumn("Actions");

                    ImGui::TableHeadersRow();

                    int id = 0;
                    for (std::string const& socketName : socketNames)
                    {
                        OpenSim::AbstractSocket const& socket = c.getSocket(socketName);

                        int column = 0;
                        ImGui::PushID(id++);
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(column++);
                        ui::TextDisabled(socketName);

                        ImGui::TableSetColumnIndex(column++);
                        if (ImGui::SmallButton(socket.getConnecteeAsObject().getName().c_str()))
                        {
                            m_Model->setSelected(dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject()));
                            requestClose();
                        }
                        if (auto const* connectee = dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject());
                            connectee && ImGui::IsItemHovered())
                        {
                            DrawComponentHoverTooltip(*connectee);
                        }

                        ImGui::TableSetColumnIndex(column++);
                        if (ImGui::SmallButton("change"))
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

                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
            }
            else
            {
                ui::TextDisabled("%s has no sockets", c.getName().c_str());
            }

            ImGui::EndMenu();
        }
    }

    void drawAddMusclePlotMenu(OpenSim::Muscle const& m)
    {
        if (ImGui::BeginMenu("Plot vs. Coordinate"))
        {
            for (OpenSim::Coordinate const& c : m_Model->getModel().getComponentList<OpenSim::Coordinate>())
            {
                if (ImGui::MenuItem(c.getName().c_str()))
                {
                    m_EditorAPI->addMusclePlot(c, m);
                }
            }

            ImGui::EndMenu();
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

bool osc::ComponentContextMenu::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::ComponentContextMenu::implOpen()
{
    m_Impl->open();
}

void osc::ComponentContextMenu::implClose()
{
    m_Impl->close();
}

bool osc::ComponentContextMenu::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::ComponentContextMenu::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::ComponentContextMenu::implEndPopup()
{
    m_Impl->endPopup();
}
