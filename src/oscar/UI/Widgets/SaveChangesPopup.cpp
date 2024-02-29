#include "SaveChangesPopup.h"

#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/SaveChangesPopupConfig.h>
#include <oscar/UI/Widgets/StandardPopup.h>

#include <utility>

class osc::SaveChangesPopup::Impl final : public StandardPopup {
public:

    explicit Impl(SaveChangesPopupConfig config) :
        StandardPopup{config.popupName},
        m_Config{std::move(config)}
    {
    }

    void implDrawContent() final
    {
        ui::TextUnformatted(m_Config.content);

        if (ui::Button("Yes"))
        {
            if (m_Config.onUserClickedSave())
            {
                close();
            }
        }

        ui::SameLine();

        if (ui::Button("No"))
        {
            if (m_Config.onUserClickedDontSave())
            {
                close();
            }
        }

        ui::SameLine();

        if (ui::Button("Cancel"))
        {
            if (m_Config.onUserCancelled())
            {
                close();
            }
        }
    }
private:
    SaveChangesPopupConfig m_Config;
};


// public API (PIMPL)

osc::SaveChangesPopup::SaveChangesPopup(SaveChangesPopupConfig const& config) :
    m_Impl{std::make_unique<Impl>(config)}
{
}

osc::SaveChangesPopup::SaveChangesPopup(SaveChangesPopup&&) noexcept = default;
osc::SaveChangesPopup& osc::SaveChangesPopup::operator=(SaveChangesPopup&&) noexcept = default;
osc::SaveChangesPopup::~SaveChangesPopup() noexcept = default;

bool osc::SaveChangesPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::SaveChangesPopup::implOpen()
{
    m_Impl->open();
}

void osc::SaveChangesPopup::implClose()
{
    m_Impl->close();
}

bool osc::SaveChangesPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::SaveChangesPopup::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::SaveChangesPopup::implEndPopup()
{
    m_Impl->endPopup();
}

void osc::SaveChangesPopup::onDraw()
{
    if (m_Impl->beginPopup())
    {
        m_Impl->onDraw();
        m_Impl->endPopup();
    }
}
