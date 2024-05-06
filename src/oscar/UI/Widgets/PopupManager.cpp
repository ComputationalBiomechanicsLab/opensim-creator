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
    popups_.push_back(std::move(popup));
}

void osc::PopupManager::open_all()
{
    for (std::shared_ptr<IPopup>& popup : popups_) {
        popup->open();
    }
}

void osc::PopupManager::on_draw()
{
    // begin and (if applicable) draw bottom-to-top in a nested fashion
    ptrdiff_t num_opened = 0;
    ptrdiff_t num_popups = std::ssize(popups_);  // only draw the popups that existed at the start of this frame, not the ones added during this frame
    for (ptrdiff_t i = 0; i < num_popups; ++i) {
        if (popups_[i]->begin_popup()) {
            popups_[i]->on_draw();
            ++num_opened;
        }
        else {
            break;
        }
    }

    // end the opened popups top-to-bottom
    for (ptrdiff_t i = num_opened-1; i >= 0; --i) {
        popups_[i]->end_popup();
    }

    // garbage-collect any closed popups
    std::erase_if(popups_, [](const auto& ptr) { return not ptr->is_open(); });
}

bool osc::PopupManager::empty()
{
    return popups_.empty();
}

void osc::PopupManager::clear()
{
    popups_.clear();
}
