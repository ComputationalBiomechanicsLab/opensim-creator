#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <memory>

namespace osc
{
    class IPopupAPI {
    protected:
        IPopupAPI() = default;
        IPopupAPI(IPopupAPI const&) = default;
        IPopupAPI(IPopupAPI&&) noexcept = default;
        IPopupAPI& operator=(IPopupAPI const&) = default;
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
