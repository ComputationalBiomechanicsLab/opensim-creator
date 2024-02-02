#include "ModelStatePairContextMenu.hpp"

#include <OpenSimCreator/Documents/Model/IModelStatePair.hpp>
#include <OpenSimCreator/UI/IMainUIStateAPI.hpp>
#include <OpenSimCreator/UI/Shared/BasicWidgets.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/UI/Widgets/StandardPopup.hpp>
#include <oscar/Utils/ParentPtr.hpp>

#include <memory>
#include <string_view>
#include <utility>


class osc::ModelStatePairContextMenu::Impl final : public StandardPopup {
public:
    Impl(
        std::string_view panelName_,
        std::shared_ptr<IModelStatePair> model_,
        ParentPtr<IMainUIStateAPI> const& api_,
        std::optional<std::string> maybeComponentAbsPath_) :

        StandardPopup{panelName_, {10.0f, 10.0f}, ImGuiWindowFlags_NoMove},
        m_Model{std::move(model_)},
        m_API{api_},
        m_MaybeComponentAbsPath{std::move(maybeComponentAbsPath_)}
    {
        setModal(false);
    }

    void implDrawContent() override
    {
        if (!m_MaybeComponentAbsPath)
        {
            drawRightClickedNothingContextMenu();
        }
        else if (OpenSim::Component const* c = osc::FindComponent(m_Model->getModel(), *m_MaybeComponentAbsPath))
        {
            drawRightClickedSomethingContextMenu(*c);
        }
        else
        {
            drawRightClickedNothingContextMenu();
        }
    }

    void drawRightClickedNothingContextMenu()
    {
        ImGui::TextDisabled("(clicked nothing)");
    }

    void drawRightClickedSomethingContextMenu(OpenSim::Component const& c)
    {
        // draw context menu for whatever's selected
        ImGui::TextUnformatted(c.getName().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("%s", c.getConcreteClassName().c_str());
        ImGui::Separator();
        ImGui::Dummy({0.0f, 3.0f});

        DrawSelectOwnerMenu(*m_Model, c);
        DrawWatchOutputMenu(*m_API, c);
        TryDrawCalculateMenu(
            m_Model->getModel(),
            m_Model->getState(),
            c,
            CalculateMenuFlags::NoCalculatorIcon
        );
    }

private:
    std::shared_ptr<IModelStatePair> m_Model;
    ParentPtr<IMainUIStateAPI> m_API;
    std::optional<std::string> m_MaybeComponentAbsPath;
};


// public API

osc::ModelStatePairContextMenu::ModelStatePairContextMenu(
    std::string_view panelName_,
    std::shared_ptr<IModelStatePair> model_,
    ParentPtr<IMainUIStateAPI> const& api_,
    std::optional<std::string> maybeComponentAbsPath_) :

    m_Impl{std::make_unique<Impl>(panelName_, std::move(model_), api_, std::move(maybeComponentAbsPath_))}
{
}

osc::ModelStatePairContextMenu::ModelStatePairContextMenu(ModelStatePairContextMenu&&) noexcept = default;
osc::ModelStatePairContextMenu& osc::ModelStatePairContextMenu::operator=(ModelStatePairContextMenu&&) noexcept = default;
osc::ModelStatePairContextMenu::~ModelStatePairContextMenu() noexcept = default;

bool osc::ModelStatePairContextMenu::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::ModelStatePairContextMenu::implOpen()
{
    m_Impl->open();
}

void osc::ModelStatePairContextMenu::implClose()
{
    m_Impl->close();
}

bool osc::ModelStatePairContextMenu::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::ModelStatePairContextMenu::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::ModelStatePairContextMenu::implEndPopup()
{
    m_Impl->endPopup();
}
