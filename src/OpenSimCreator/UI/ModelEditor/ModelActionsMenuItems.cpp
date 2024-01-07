#include "ModelActionsMenuItems.hpp"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.hpp>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.hpp>
#include <OpenSimCreator/UI/ModelEditor/AddBodyPopup.hpp>
#include <OpenSimCreator/UI/ModelEditor/AddComponentPopup.hpp>
#include <OpenSimCreator/UI/ModelEditor/EditorAPI.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Control/Controller.h>
#include <OpenSim/Simulation/Model/Actuator.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Probe.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>
#include <utility>

class osc::ModelActionsMenuItems::Impl final {
public:

    Impl(
        EditorAPI* api,
        std::shared_ptr<UndoableModelStatePair> uum_) :

        m_EditorAPI{api},
        m_Model{std::move(uum_)}
    {
    }

    void onDraw()
    {
        ImGui::PushID(this);

        // action: add body
        {
            // draw button
            if (ImGui::MenuItem("Body"))
            {
                auto popup = std::make_unique<AddBodyPopup>("add body", m_EditorAPI, m_Model);
                popup->open();
                m_EditorAPI->pushPopup(std::move(popup));
            }

            // draw tooltip (if hovered)
            if (ImGui::IsItemHovered())
            {
                DrawTooltip(
                    "Add an OpenSim::Body into the model",
                    "An OpenSim::Body is a PhysicalFrame (reference frame) with an associated inertia specified by its mass, center-of-mass located in the PhysicalFrame, and its moment of inertia tensor about the center-of-mass");
            }
        }

        renderButton<OpenSim::Joint>();
        renderButton<OpenSim::ContactGeometry>();
        renderButton<OpenSim::Constraint>();
        renderButton<OpenSim::Force>();
        renderButton<OpenSim::Controller>();
        renderButton<OpenSim::Probe>();
        renderButton<OpenSim::Component>();

        ImGui::PopID();
    }

private:

    template<typename T>
    void renderButton()
    {
        ComponentRegistry<T> const& registry = osc::GetComponentRegistry<T>();

        if (ImGui::BeginMenu(registry.name().c_str()))
        {
            for (ComponentRegistryEntry<T> const& entry : registry)
            {
                if (ImGui::MenuItem(entry.name().c_str()))
                {
                    auto popup = std::make_unique<AddComponentPopup>(
                        "Add " + registry.name(),
                        m_EditorAPI,
                        m_Model,
                        entry.instantiate()
                    );
                    popup->open();
                    m_EditorAPI->pushPopup(std::move(popup));
                }

                if (ImGui::IsItemHovered())
                {
                    DrawTooltip(entry.name(), entry.description());
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::IsItemHovered())
        {
            std::stringstream ttTitle;
            ttTitle << "Add a " << registry.name() << " into the model";

            DrawTooltip(ttTitle.str(), registry.description());
        }
    }

    EditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;
};


// public API (PIMPL)

osc::ModelActionsMenuItems::ModelActionsMenuItems(EditorAPI* api, std::shared_ptr<UndoableModelStatePair> m) :
    m_Impl{std::make_unique<Impl>(api, std::move(m))}
{
}

osc::ModelActionsMenuItems::ModelActionsMenuItems(ModelActionsMenuItems&&) noexcept = default;
osc::ModelActionsMenuItems& osc::ModelActionsMenuItems::operator=(ModelActionsMenuItems&&) noexcept = default;
osc::ModelActionsMenuItems::~ModelActionsMenuItems() noexcept = default;

void osc::ModelActionsMenuItems::onDraw()
{
    m_Impl->onDraw();
}
