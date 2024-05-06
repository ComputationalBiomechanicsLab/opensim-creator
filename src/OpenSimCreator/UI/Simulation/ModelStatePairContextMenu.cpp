#include "ModelStatePairContextMenu.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/ParentPtr.h>

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
        set_modal(false);
    }

    void impl_draw_content() override
    {
        if (!m_MaybeComponentAbsPath)
        {
            drawRightClickedNothingContextMenu();
        }
        else if (OpenSim::Component const* c = FindComponent(m_Model->getModel(), *m_MaybeComponentAbsPath))
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
        ui::TextDisabled("(clicked nothing)");
    }

    void drawRightClickedSomethingContextMenu(OpenSim::Component const& c)
    {
        // draw context menu for whatever's selected
        ui::TextUnformatted(c.getName());
        ui::SameLine();
        ui::TextDisabled(c.getConcreteClassName());
        ui::Separator();
        ui::Dummy({0.0f, 3.0f});

        DrawSelectOwnerMenu(*m_Model, c);
        DrawWatchOutputMenu(c, [this](OpenSim::AbstractOutput const& output, std::optional<ComponentOutputSubfield> subfield)
        {
            if (subfield) {
                m_API->addUserOutputExtractor(OutputExtractor{ComponentOutputExtractor{output, *subfield}});
            }
            else {
                m_API->addUserOutputExtractor(OutputExtractor{ComponentOutputExtractor{output}});
            }

        });
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

bool osc::ModelStatePairContextMenu::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::ModelStatePairContextMenu::impl_open()
{
    m_Impl->open();
}

void osc::ModelStatePairContextMenu::impl_close()
{
    m_Impl->close();
}

bool osc::ModelStatePairContextMenu::impl_begin_popup()
{
    return m_Impl->begin_popup();
}

void osc::ModelStatePairContextMenu::impl_on_draw()
{
    m_Impl->on_draw();
}

void osc::ModelStatePairContextMenu::impl_end_popup()
{
    m_Impl->end_popup();
}
