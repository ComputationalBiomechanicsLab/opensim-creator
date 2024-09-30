#include "ModelStatePairContextMenu.h"

#include <OpenSimCreator/Documents/Model/Environment.h>
#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/UI/MainUIScreen.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/LifetimedPtr.h>

#include <memory>
#include <string_view>
#include <utility>


class osc::ModelStatePairContextMenu::Impl final : public StandardPopup {
public:
    Impl(
        std::string_view panelName_,
        std::shared_ptr<IModelStatePair> model_,
        std::optional<std::string> maybeComponentAbsPath_) :

        StandardPopup{panelName_, {10.0f, 10.0f}, ui::WindowFlag::NoMove},
        m_Model{std::move(model_)},
        m_MaybeComponentAbsPath{std::move(maybeComponentAbsPath_)}
    {
        set_modal(false);
    }

    void impl_draw_content() override
    {
        if (!m_MaybeComponentAbsPath) {
            drawRightClickedNothingContextMenu();
        }
        else if (const OpenSim::Component* c = FindComponent(m_Model->getModel(), *m_MaybeComponentAbsPath)) {
            drawRightClickedSomethingContextMenu(*c);
        }
        else {
            drawRightClickedNothingContextMenu();
        }
    }

    void drawRightClickedNothingContextMenu()
    {
        ui::draw_text_disabled("(clicked nothing)");
    }

    void drawRightClickedSomethingContextMenu(const OpenSim::Component& c)
    {
        // draw context menu for whatever's selected
        ui::draw_text_unformatted(c.getName());
        ui::same_line();
        ui::draw_text_disabled(c.getConcreteClassName());
        ui::draw_separator();
        ui::draw_dummy({0.0f, 3.0f});

        DrawSelectOwnerMenu(*m_Model, c);
        DrawWatchOutputMenu(c, [this](const OpenSim::AbstractOutput& output, std::optional<ComponentOutputSubfield> subfield)
        {
            std::shared_ptr<Environment> environment = m_Model->tryUpdEnvironment();
            if (subfield) {
                environment->addUserOutputExtractor(OutputExtractor{ComponentOutputExtractor{output, *subfield}});
            }
            else {
                environment->addUserOutputExtractor(OutputExtractor{ComponentOutputExtractor{output}});
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
    std::optional<std::string> m_MaybeComponentAbsPath;
};


osc::ModelStatePairContextMenu::ModelStatePairContextMenu(
    std::string_view panelName_,
    std::shared_ptr<IModelStatePair> model_,
    std::optional<std::string> maybeComponentAbsPath_) :

    m_Impl{std::make_unique<Impl>(panelName_, std::move(model_), std::move(maybeComponentAbsPath_))}
{}
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
