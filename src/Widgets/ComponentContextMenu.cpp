#include "ComponentContextMenu.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/MiddlewareAPIs/EditorAPI.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Widgets/SelectComponentPopup.hpp"
#include "src/Widgets/Select1PFPopup.hpp"
#include "src/Widgets/SelectGeometryPopup.hpp"
#include "src/Widgets/StandardPopup.hpp"

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

    int idx = osc::FindJointInParentJointSet(*joint);

    if (idx == -1)
    {
        return;
    }

    // look the Joint up in the type registry so we know where it should be in the ImGui::Combo
    std::optional<size_t> maybeTypeIndex = osc::JointRegistry::indexOf(*joint);
    int typeIndex = maybeTypeIndex ? static_cast<int>(*maybeTypeIndex) : -1;

    auto jointNames = osc::JointRegistry::nameCStrings();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    if (ImGui::Combo(
        "joint type",
        &typeIndex,
        jointNames.data(),
        static_cast<int>(jointNames.size())) &&
        typeIndex >= 0)
    {
        // copy + fixup  a prototype of the user's selection
        std::unique_ptr<OpenSim::Joint> newJoint{osc::JointRegistry::prototypes()[static_cast<size_t>(typeIndex)]->clone()};
        osc::ActionChangeJointTypeTo(uim, jointPath, std::move(newJoint));
    }
}

// draw contextual actions (buttons, sliders) for a selected physical frame
static void DrawPhysicalFrameContextualActions(
    osc::EditorAPI* editorAPI,
    std::shared_ptr<osc::UndoableModelStatePair> uim,
    OpenSim::ComponentPath pfPath)
{
    if (ImGui::MenuItem("add geometry"))
    {
        std::function<void(std::unique_ptr<OpenSim::Geometry>)> callback = [uim, pfPath](auto geom) { osc::ActionAttachGeometryToPhysicalFrame(*uim, pfPath, std::move(geom)); };
        std::unique_ptr<osc::Popup> p = std::make_unique<osc::SelectGeometryPopup>("select geometry to attach", callback);
        p->open();
        editorAPI->pushPopup(std::move(p));
    }
    osc::DrawTooltipIfItemHovered("Add Geometry", "Add geometry to this component. Geometry can be removed by selecting it in the hierarchy editor and pressing DELETE");

    if (ImGui::MenuItem("add offset frame"))
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
        if (ImGui::MenuItem("rezero joint"))
        {
            osc::ActionRezeroJoint(uim, jointPath);
        }
        osc::DrawTooltipIfItemHovered("Re-zero the joint", "Given the joint's current geometry due to joint defaults, coordinate defaults, and any coordinate edits made in the coordinate editor, this will reorient the joint's parent (if it's an offset frame) to match the child's transformation. Afterwards, it will then resets all of the joints coordinates to zero. This effectively sets the 'zero point' of the joint (i.e. the geometry when all coordinates are zero) to match whatever the current geometry is.");
    }

    if (ImGui::MenuItem("add parent offset frame"))
    {
        osc::ActionAddParentOffsetFrameToJoint(uim, jointPath);
    }

    if (ImGui::MenuItem("add child offset frame"))
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

    if (ImGui::MenuItem("add contact geometry"))
    {
        auto onSelection = [uim, hcfPath](OpenSim::ComponentPath const& geomPath)
        {
            osc::ActionAssignContactGeometryToHCF(*uim, hcfPath, geomPath);
        };
        auto filter = [](OpenSim::Component const& c) -> bool
        {
            return dynamic_cast<OpenSim::ContactGeometry const*>(&c);
        };
        auto popup = std::make_unique<osc::SelectComponentPopup>("select contact geometry", uim, onSelection, filter);
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
    char const* modalName = "select physical frame";

    if (ImGui::MenuItem("add path point"))
    {
        auto onSelection = [uim, paPath](OpenSim::ComponentPath const& pfPath) { osc::ActionAddPathPointToPathActuator(*uim, paPath, pfPath); };
        auto popup = std::make_unique<osc::Select1PFPopup>("select physical frame", uim, onSelection);
        popup->open();
        api->pushPopup(std::move(popup));
    }
    osc::DrawTooltipIfItemHovered("Add Path Point", "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::PathActuator");
}

static void DrawModelContextualActions(osc::UndoableModelStatePair& uim)
{
    if (ImGui::MenuItem("toggle frames"))
    {
        osc::ActionToggleFrames(uim);
    }
}

class osc::ComponentContextMenu::Impl final : public osc::StandardPopup {
public:
    Impl(std::string_view popupName,
         EditorAPI* editor,
         std::shared_ptr<UndoableModelStatePair> model,
         OpenSim::ComponentPath const& path) :

        StandardPopup{popupName, 10.0f, 10.0f, ImGuiWindowFlags_NoMove},
        m_EditorAPI{std::move(editor)},
        m_Model{std::move(model)},
        m_Path{path}
    {
        setModal(false);
        OSC_ASSERT(m_Model != nullptr);
    }

private:
    void implDraw() override
    {
        OpenSim::Component const* c = osc::FindComponent(m_Model->getModel(), m_Path);
        if (!c)
        {
            ImGui::TextDisabled("(cannot find %s in the model)", m_Path.toString().c_str());
            return;
        }

        if (c != m_Model->getIsolated())
        {
            if (ImGui::MenuItem("isolate"))
            {
                osc::ActionSetModelIsolationTo(*m_Model, c);
            }
        }
        else
        {
            if (ImGui::MenuItem("clear isolation"))
            {
                osc::ActionSetModelIsolationTo(*m_Model, nullptr);
            }
        }
        osc::DrawTooltipIfItemHovered("Toggle Isolation", "Only show this component in the visualizer\n\nThis can be disabled from the Edit menu (Edit -> Clear Isolation)");

        if (ImGui::MenuItem("copy absolute path to clipboard"))
        {
            std::string path = c->getAbsolutePathString();
            osc::SetClipboardText(path.c_str());
        }
        osc::DrawTooltipIfItemHovered("Copy Component Absolute Path", "Copy the absolute path to this component to your clipboard.\n\n(This is handy if you are separately using absolute component paths to (e.g.) manipulate the model in a script or something)");

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
        else if (auto const* pa = dynamic_cast<OpenSim::PathActuator const*>(c))
        {
            DrawPathActuatorContextualParams(m_EditorAPI, m_Model, m_Path);
        }
    }

    EditorAPI* m_EditorAPI = nullptr;
    std::shared_ptr<UndoableModelStatePair> m_Model;
    OpenSim::ComponentPath m_Path;
};


// public API (PIMPL)

osc::ComponentContextMenu::ComponentContextMenu(
    std::string_view popupName,
    EditorAPI* api,
    std::shared_ptr<UndoableModelStatePair> model,
    OpenSim::ComponentPath const& path) :

    m_Impl{new Impl{std::move(popupName), std::move(api), std::move(model), path}}
{
}

osc::ComponentContextMenu::ComponentContextMenu(ComponentContextMenu&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ComponentContextMenu& osc::ComponentContextMenu::operator=(ComponentContextMenu&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ComponentContextMenu::~ComponentContextMenu() noexcept
{
    delete m_Impl;
}

bool osc::ComponentContextMenu::isOpen() const
{
    return m_Impl->isOpen();
}

void osc::ComponentContextMenu::open()
{
    m_Impl->open();
}

void osc::ComponentContextMenu::close()
{
    m_Impl->close();
}

void osc::ComponentContextMenu::draw()
{
    m_Impl->draw();
}