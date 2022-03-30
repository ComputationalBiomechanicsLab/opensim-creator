#include "ModelActionsMenuBar.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UndoableUiModel.hpp"
#include "src/Widgets/AddBodyPopup.hpp"
#include "src/Widgets/AddComponentPopup.hpp"
#include "src/Widgets/Select2PFsPopup.hpp"

#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <optional>
#include <sstream>
#include <utility>

class osc::ModelActionsMenuBar::Impl final {
public:
    Impl(std::shared_ptr<UndoableUiModel> uum_) :
        m_Uum{uum_},
        m_AddBodyPopup{uum_, "add body"},
        m_Select2PFsPopup{},
        m_MaybeAddComponentPopup{std::nullopt}
    {
    }

    bool draw()
    {
        if (ImGui::BeginMenuBar())
        {
            return renderMenuBarContent();
            ImGui::EndMenuBar();
        }
        else
        {
            return false;
        }
    }

private:

    template<typename T>
    void renderButton()
    {
        std::stringstream label;
        label << ICON_FA_PLUS << " Add " << osc::TypeRegistry<T>::name();

        // action: add joint
        if (ImGui::BeginMenu(label.str().c_str()))
        {
            auto names = osc::TypeRegistry<T>::nameCStrings();

            for (size_t i = 0; i < names.size(); ++i)
            {
                if (ImGui::MenuItem(names[i]))
                {
                    std::unique_ptr<T> copy{osc::TypeRegistry<T>::prototypes()[i]->clone()};
                    m_MaybeAddComponentPopup = osc::AddComponentPopup{m_Uum, std::move(copy), "Add " + osc::TypeRegistry<T>::name()};
                    m_MaybeAddComponentPopup->open();
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

    bool renderMenuBarContent()
    {
        bool editMade = false;

        // action: add body
        {
            // draw button
            if (ImGui::MenuItem(ICON_FA_PLUS " Add Body"))
            {
                m_AddBodyPopup.open();
            }

            // draw tooltip (if hovered)
            if (ImGui::IsItemHovered())
            {
                DrawTooltip(
                    "Add an OpenSim::Body into the model",
                    "An OpenSim::Body is a PhysicalFrame (reference frame) with an associated inertia specified by its mass, center-of-mass located in the PhysicalFrame, and its moment of inertia tensor about the center-of-mass");
            }

            if (m_AddBodyPopup.draw())
            {
                editMade = true;
            }
        }

        renderButton<OpenSim::Joint>();
        renderButton<OpenSim::ContactGeometry>();
        renderButton<OpenSim::Constraint>();
        renderButton<OpenSim::Force>();
        renderButton<OpenSim::Controller>();
        renderButton<OpenSim::Probe>();
        renderButton<OpenSim::Component>();

        if (m_MaybeAddComponentPopup)
        {
            editMade = m_MaybeAddComponentPopup->draw();
        }

        return editMade;
    }

    std::shared_ptr<UndoableUiModel> m_Uum;
    AddBodyPopup m_AddBodyPopup;
    Select2PFsPopup m_Select2PFsPopup;
    std::optional<AddComponentPopup> m_MaybeAddComponentPopup;
};

osc::ModelActionsMenuBar::ModelActionsMenuBar(std::shared_ptr<UndoableUiModel> m) :
    m_Impl{std::make_unique<Impl>(std::move(m))}
{
}
osc::ModelActionsMenuBar::ModelActionsMenuBar(ModelActionsMenuBar&&) noexcept = default;
osc::ModelActionsMenuBar& osc::ModelActionsMenuBar::operator=(ModelActionsMenuBar&&) noexcept = default;
osc::ModelActionsMenuBar::~ModelActionsMenuBar() noexcept = default;

bool osc::ModelActionsMenuBar::draw()
{
    return m_Impl->draw();
}
