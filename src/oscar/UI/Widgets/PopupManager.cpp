#include "PopupManager.h"

#include <oscar/UI/Widgets/IPopup.h>

#include <cstddef>
#include <memory>
#include <vector>

osc::PopupManager::PopupManager() = default;
osc::PopupManager::PopupManager(PopupManager&&) noexcept = default;
osc::PopupManager& osc::PopupManager::operator=(PopupManager&&) noexcept = default;
osc::PopupManager::~PopupManager() noexcept = default;

void osc::PopupManager::push_back(std::shared_ptr<IPopup> popup)
{
    m_Popups.push_back(std::move(popup));
}

void osc::PopupManager::openAll()
{
    for (std::shared_ptr<IPopup>& popup : m_Popups)
    {
        popup->open();
    }
}

void osc::PopupManager::onDraw()
{
    // begin and (if applicable) draw bottom-to-top in a nested fashion
    ptrdiff_t nOpened = 0;
    ptrdiff_t nPopups = std::ssize(m_Popups);  // only draw the popups that existed at the start of this frame, not the ones added during this frame
    for (ptrdiff_t i = 0; i < nPopups; ++i)
    {
        if (m_Popups[i]->beginPopup())
        {
            m_Popups[i]->onDraw();
            ++nOpened;
        }
        else
        {
            break;
        }
    }

    // end the opened popups top-to-bottom
    for (ptrdiff_t i = nOpened-1; i >= 0; --i)
    {
        m_Popups[i]->endPopup();
    }

    // garbage-collect any closed popups
    std::erase_if(m_Popups, [](const auto& ptr) { return !ptr->isOpen(); });
}

bool osc::PopupManager::empty()
{
    return m_Popups.empty();
}

void osc::PopupManager::clear()
{
    m_Popups.clear();
}
