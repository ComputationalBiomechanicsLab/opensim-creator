#include "ModelActionsMenuItems.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/MiddlewareAPIs/EditorAPI.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Widgets/AddBodyPopup.hpp"
#include "src/Widgets/AddComponentPopup.hpp"

#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Control/Controller.h>
#include <OpenSim/Simulation/Model/Actuator.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Probe.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>
#include <utility>

class osc::ModelActionsMenuItems::Impl final {
public:

    Impl(EditorAPI* api, std::shared_ptr<UndoableModelStatePair> uum_) :
        m_EditorAPI{std::move(api)},
        m_Uum{uum_}
    {
    }

    void draw()
    {
        ImGui::PushID(this);

        // action: add body
        {
            // draw button
            if (ImGui::MenuItem("Body"))
            {
                auto popup = std::make_unique<AddBodyPopup>(m_EditorAPI, m_Uum, "add body");
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
        std::stringstream label;
        label << osc::TypeRegistry<T>::name();

        // action: add joint
        if (ImGui::BeginMenu(label.str().c_str()))
        {
            auto names = osc::TypeRegistry<T>::nameCStrings();

            for (size_t i = 0; i < names.size(); ++i)
            {
                if (ImGui::MenuItem(names[i]))
                {
                    std::unique_ptr<T> copy{osc::TypeRegistry<T>::prototypes()[i]->clone()};
                    auto popup = std::make_unique<AddComponentPopup>(m_EditorAPI, m_Uum, std::move(copy), "Add " + osc::TypeRegistry<T>::name());
                    popup->open();
                    m_EditorAPI->pushPopup(std::move(popup));
                }

                if (ImGui::IsItemHovered())
                {
                    DrawTooltip(names[i], osc::TypeRegistry<T>::descriptionCStrings()[i]);
                }
            }

            ImGui::EndMenu();
        }
        if (ImGui::IsItemHovered())
        {
            std::stringstream ttTitle;
            ttTitle << "Add a " << osc::TypeRegistry<T>::name() << " into the model";
            DrawTooltip(
                ttTitle.str().c_str(),
                osc::TypeRegistry<T>::description().c_str());
        }
    }

    EditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Uum;
};


// public API

osc::ModelActionsMenuItems::ModelActionsMenuItems(EditorAPI* api, std::shared_ptr<UndoableModelStatePair> m) :
    m_Impl{new Impl{std::move(api), std::move(m)}}
{
}

osc::ModelActionsMenuItems::ModelActionsMenuItems(ModelActionsMenuItems&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ModelActionsMenuItems& osc::ModelActionsMenuItems::operator=(ModelActionsMenuItems&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ModelActionsMenuItems::~ModelActionsMenuItems() noexcept
{
    delete m_Impl;
}

void osc::ModelActionsMenuItems::draw()
{
    m_Impl->draw();
}
