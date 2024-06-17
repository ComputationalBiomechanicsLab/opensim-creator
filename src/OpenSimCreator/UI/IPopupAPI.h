#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <memory>

namespace osc
{
    class IPopupAPI {
    protected:
        IPopupAPI() = default;
        IPopupAPI(const IPopupAPI&) = default;
        IPopupAPI(IPopupAPI&&) noexcept = default;
        IPopupAPI& operator=(const IPopupAPI&) = default;
        IPopupAPI& operator=(IPopupAPI&&) noexcept = default;
    public:
        virtual ~IPopupAPI() noexcept = default;

        void pushPopup(std::unique_ptr<IPopup> p)
        {
            implPushPopup(std::move(p));
        }
    private:
        virtual void implPushPopup(std::unique_ptr<IPopup>) = 0;
    };
}
