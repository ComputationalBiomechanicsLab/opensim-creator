#include "ComponentContextMenu.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/EditorAPI.hpp"
#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/Widgets/ModelActionsMenuItems.hpp"
#include "src/OpenSimBindings/Widgets/ReassignSocketPopup.hpp"
#include "src/OpenSimBindings/Widgets/SelectComponentPopup.hpp"
#include "src/OpenSimBindings/Widgets/Select1PFPopup.hpp"
#include "src/OpenSimBindings/Widgets/SelectGeometryPopup.hpp"
#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Widgets/StandardPopup.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathActuator.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>

#include <memory>
#include <utility>

// draw UI element that lets user change a model joint's type
static void DrawSelectionJointTypeSwitcher(
    osc::UndoableModelStatePair& uim,
    OpenSim::ComponentPath jointPath)
{
    OpenSim::Joint const* joint = osc::FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
    if (!joint)
    {
        return;
    }

    std::optional<int> maybeIdx = osc::FindJointInParentJointSet(*joint);

    if (!maybeIdx)
    {
        return;
    }

    int selectedIdx = -1;
    if (ImGui::BeginMenu("Change Joint Type"))
    {
        // look the Joint up in the type registry so we know where it should be in the ImGui::Combo
        std::optional<size_t> maybeTypeIndex = osc::JointRegistry::indexOf(*joint);
        int typeIndex = maybeTypeIndex ? static_cast<int>(*maybeTypeIndex) : -1;
        auto jointNames = osc::JointRegistry::nameCStrings();

        for (int i = 0; i < static_cast<int>(jointNames.size()); ++i)
        {
            bool selected = i == typeIndex;
            bool wasSelected = selected;
            if (ImGui::MenuItem(jointNames[i], nullptr, &selected))
            {
                if (!wasSelected)
                {
                    selectedIdx = i;
                }
            }
        }
        ImGui::EndMenu();
    }

    if (0 <= selectedIdx && selectedIdx < osc::JointRegistry::prototypes().size())
    {
        // copy + fixup  a prototype of the user's selection
        std::unique_ptr<OpenSim::Joint> newJoint{osc::JointRegistry::prototypes()[static_cast<size_t>(selectedIdx)]->clone()};
        osc::ActionChangeJointTypeTo(uim, jointPath, std::move(newJoint));
    }
}

// draw contextual actions (buttons, sliders) for a selected physical frame
static void DrawPhysicalFrameContextualActions(
    osc::EditorAPI* editorAPI,
    std::shared_ptr<osc::UndoableModelStatePair> uim,
    OpenSim::ComponentPath pfPath)
{
    if (ImGui::MenuItem("Add Geometry"))
    {
        std::function<void(std::unique_ptr<OpenSim::Geometry>)> callback = [uim, pfPath](auto geom) { osc::ActionAttachGeometryToPhysicalFrame(*uim, pfPath, std::move(geom)); };
        std::unique_ptr<osc::Popup> p = std::make_unique<osc::SelectGeometryPopup>("select geometry to attach", callback);
        p->open();
        editorAPI->pushPopup(std::move(p));
    }
    osc::DrawTooltipIfItemHovered("Add Geometry", "Add geometry to this component. Geometry can be removed by selecting it in the navigator and pressing DELETE");

    if (ImGui::MenuItem("Add Offset Frame"))
    {
        osc::ActionAddOffsetFrameToPhysicalFrame(*uim, pfPath);
    }
    osc::DrawTooltipIfItemHovered("Add Offset Frame", "Add an OpenSim::OffsetFrame as a child of this Component. Other components in the model can then connect to this OffsetFrame, rather than the base Component, so that it can connect at some offset that is relative to the parent Component");
}


// draw contextual actions (buttons, sliders) for a selected joint
static void DrawJointContextualActions(
    osc::UndoableModelStatePair& uim,
    OpenSim::ComponentPath jointPath)
{
    DrawSelectionJointTypeSwitcher(uim, jointPath);

    if (CanRezeroJoint(uim, jointPath))
    {
        if (ImGui::MenuItem("Rezero Joint"))
        {
            osc::ActionRezeroJoint(uim, jointPath);
        }
        osc::DrawTooltipIfItemHovered("Re-zero the joint", "Given the joint's current geometry due to joint defaults, coordinate defaults, and any coordinate edits made in the coordinates panel, this will reorient the joint's parent (if it's an offset frame) to match the child's transformation. Afterwards, it will then resets all of the joints coordinates to zero. This effectively sets the 'zero point' of the joint (i.e. the geometry when all coordinates are zero) to match whatever the current geometry is.");
    }

    if (ImGui::MenuItem("Add Parent Offset Frame"))
    {
        osc::ActionAddParentOffsetFrameToJoint(uim, jointPath);
    }

    if (ImGui::MenuItem("Add Child Offset Frame"))
    {
        osc::ActionAddChildOffsetFrameToJoint(uim, jointPath);
    }
}

// draw contextual actions (buttons, sliders) for a selected joint
static void DrawHCFContextualActions(
    osc::EditorAPI* api,
    std::shared_ptr<osc::UndoableModelStatePair> uim,
    OpenSim::ComponentPath hcfPath)
{
    OpenSim::HuntCrossleyForce const* hcf = osc::FindComponent<OpenSim::HuntCrossleyForce>(uim->getModel(), hcfPath);
    if (!hcf)
    {
        return;
    }

    if (hcf->get_contact_parameters().getSize() > 1)
    {
        return;  // cannot edit: has more than one HuntCrossleyForce::Parameter
    }

    if (ImGui::MenuItem("Add Contact Geometry"))
    {
        auto onSelection = [uim, hcfPath](OpenSim::ComponentPath const& geomPath)
        {
            osc::ActionAssignContactGeometryToHCF(*uim, hcfPath, geomPath);
        };
        auto filter = [](OpenSim::Component const& c) -> bool
        {
            return dynamic_cast<OpenSim::ContactGeometry const*>(&c);
        };
        auto popup = std::make_unique<osc::SelectComponentPopup>("Select Contact Geometry", uim, onSelection, filter);
        popup->open();
        api->pushPopup(std::move(popup));
    }
    osc::DrawTooltipIfItemHovered("Add Contact Geometry", "Add OpenSim::ContactGeometry to this OpenSim::HuntCrossleyForce.\n\nCollisions are evaluated for all OpenSim::ContactGeometry attached to the OpenSim::HuntCrossleyForce. E.g. if you want an OpenSim::ContactSphere component to collide with an OpenSim::ContactHalfSpace component during a simulation then you should add both of those components to this force");
}

// draw contextual actions (buttons, sliders) for a selected path actuator
static void DrawPathActuatorContextualParams(
    osc::EditorAPI* api,
    std::shared_ptr<osc::UndoableModelStatePair> uim,
    OpenSim::ComponentPath paPath)
{
    if (ImGui::MenuItem("Add Path Point"))
    {
        auto onSelection = [uim, paPath](OpenSim::ComponentPath const& pfPath) { osc::ActionAddPathPointToPathActuator(*uim, paPath, pfPath); };
        auto popup = std::make_unique<osc::Select1PFPopup>("Select Physical Frame", uim, onSelection);
        popup->open();
        api->pushPopup(std::move(popup));
    }
    osc::DrawTooltipIfItemHovered("Add Path Point", "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::PathActuator");
}

static void DrawModelContextualActions(osc::UndoableModelStatePair& uim)
{
    if (ImGui::MenuItem("Toggle Frames"))
    {
        osc::ActionToggleFrames(uim);
    }
}

class osc::ComponentContextMenu::Impl final : public osc::StandardPopup {
public:
    Impl(std::string_view popupName,
         MainUIStateAPI* mainUIStateAPI,
         EditorAPI* editorAPI,
         std::shared_ptr<UndoableModelStatePair> model,
         OpenSim::ComponentPath const& path) :

        StandardPopup{popupName, {10.0f, 10.0f}, ImGuiWindowFlags_NoMove},
        m_MainUIStateAPI{std::move(mainUIStateAPI)},
        m_EditorAPI{std::move(editorAPI)},
        m_Model{std::move(model)},
        m_Path{path}
    {
        setModal(false);
        OSC_ASSERT(m_Model != nullptr);
    }

private:
    void implDrawContent() override
    {
        OpenSim::Component const* c = osc::FindComponent(m_Model->getModel(), m_Path);
        if (!c)
        {
            // draw context menu content that's shown when nothing was right-clicked
            ImGui::TextDisabled("(nothing selected)");
            ImGui::Separator();
            ImGui::Dummy({0.0f, 3.0f});
            if (ImGui::BeginMenu("Add"))
            {
                m_ModelActionsMenuBar.draw();
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
                    osc::ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, osc::GetRootComponentPath(), true);
                }
                osc::DrawTooltipIfItemHovered("Show All", "Sets the visiblity of all components within the model to 'visible', handy for undoing selective hiding etc.");
                ImGui::EndMenu();
            }
            return;
        }

        ImGui::TextUnformatted(osc::Ellipsis(c->getName(), 15).c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("%s", c->getConcreteClassName().c_str());
        ImGui::Separator();
        ImGui::Dummy({0.0f, 3.0f});

        //DrawSelectOwnerMenu(*m_Model, *c);
        DrawWatchOutputMenu(*m_MainUIStateAPI, *c);

        if (ImGui::BeginMenu("Display"))
        {
            if (ImGui::MenuItem("Show"))
            {
                osc::ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, c->getAbsolutePath(), true);
            }
            if (ImGui::MenuItem("Show Only This"))
            {
                osc::ActionShowOnlyComponentAndAllChildren(*m_Model, c->getAbsolutePath());
            }
            if (ImGui::MenuItem("Hide"))
            {
                osc::ActionSetComponentAndAllChildrensIsVisibleTo(*m_Model, c->getAbsolutePath(), false);
            }
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Copy Absolute Path to Clipboard"))
        {
            std::string path = c->getAbsolutePathString();
            osc::SetClipboardText(path.c_str());
        }
        osc::DrawTooltipIfItemHovered("Copy Component Absolute Path", "Copy the absolute path to this component to your clipboard.\n\n(This is handy if you are separately using absolute component paths to (e.g.) manipulate the model in a script or something)");

        drawSocketMenu(*c);

        if (dynamic_cast<OpenSim::Model const*>(c))
        {
            DrawModelContextualActions(*m_Model);
        }
        else if (auto const* pf = dynamic_cast<OpenSim::PhysicalFrame const*>(c))
        {
            DrawPhysicalFrameContextualActions(m_EditorAPI, m_Model, m_Path);
        }
        else if (auto const* joint = dynamic_cast<OpenSim::Joint const*>(c))
        {
            DrawJointContextualActions(*m_Model, m_Path);
        }
        else if (auto const* hcf = dynamic_cast<OpenSim::HuntCrossleyForce const*>(c))
        {
            DrawHCFContextualActions(m_EditorAPI, m_Model, m_Path);
        }
        else if (OpenSim::Muscle const* m = dynamic_cast<OpenSim::Muscle const*>(c))
        {
            drawAddMusclePlotMenu(*m);
            DrawPathActuatorContextualParams(m_EditorAPI, m_Model, m_Path);  // a muscle is a path actuator
        }
        else if (auto const* pa = dynamic_cast<OpenSim::PathActuator const*>(c))
        {
            DrawPathActuatorContextualParams(m_EditorAPI, m_Model, m_Path);
        }
    }

    void drawSocketMenu(OpenSim::Component const& c)
    {
        if (ImGui::BeginMenu("Sockets"))
        {
            std::vector<std::string> socketNames = osc::GetSocketNames(c);

            if (!socketNames.empty())
            {
                if (ImGui::BeginTable("sockets table", 3, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Socket Name");
                    ImGui::TableSetupColumn("Connectee Name");
                    ImGui::TableSetupColumn("Actions");

                    int id = 0;
                    for (std::string const& socketName : socketNames)
                    {
                        OpenSim::AbstractSocket const& socket = c.getSocket(socketName);

                        int column = 0;
                        ImGui::PushID(id++);
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(column++);
                        ImGui::TextDisabled("%s", socketName.c_str());

                        ImGui::TableSetColumnIndex(column++);
                        if (ImGui::SmallButton(socket.getConnecteeAsObject().getName().c_str()))
                        {
                            m_Model->setSelected(dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject()));
                            requestClose();
                        }
                        if (ImGui::IsItemHovered())
                        {
                            m_Model->setHovered(dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject()));
                            osc::DrawTooltipBodyOnly("Click to select");
                        }

                        ImGui::TableSetColumnIndex(column++);
                        if (ImGui::SmallButton(ICON_FA_EDIT))
                        {
                            auto popup = std::make_unique<ReassignSocketPopup>(
                                "Reassign " + socket.getName(),
                                m_Model,
                                c.getAbsolutePathString(),
                                socketName
                            );
                            popup->open();
                            m_EditorAPI->pushPopup(std::move(popup));
                        }
                        if (ImGui::IsItemHovered())
                        {
                            osc::DrawTooltipBodyOnly("Click to edit");
                        }

                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }
            }
            else
            {
                ImGui::TextDisabled("%s has no sockets", c.getName().c_str());
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

    MainUIStateAPI* m_MainUIStateAPI = nullptr;
    EditorAPI* m_EditorAPI = nullptr;
    std::shared_ptr<UndoableModelStatePair> m_Model;
    OpenSim::ComponentPath m_Path;
    ModelActionsMenuItems m_ModelActionsMenuBar{m_EditorAPI, m_Model};
};


// public API (PIMPL)

osc::ComponentContextMenu::ComponentContextMenu(
    std::string_view popupName,
    MainUIStateAPI* mainUIStateAPI,
    EditorAPI* editorAPI,
    std::shared_ptr<UndoableModelStatePair> model,
    OpenSim::ComponentPath const& path) :

    m_Impl{std::make_unique<Impl>(std::move(popupName), std::move(mainUIStateAPI), std::move(editorAPI), std::move(model), path)}
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

void osc::ComponentContextMenu::implDrawPopupContent()
{
    m_Impl->drawPopupContent();
}

void osc::ComponentContextMenu::implEndPopup()
{
    m_Impl->endPopup();
}
