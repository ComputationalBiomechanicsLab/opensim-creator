#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class Screenshot; }
namespace osc { class ITabHost; }

namespace osc
{
    class ScreenshotTab final : public ITab {
    public:
        ScreenshotTab(const ParentPtr<ITabHost>&, Screenshot&&);
        ScreenshotTab(const ScreenshotTab&) = delete;
        ScreenshotTab(ScreenshotTab&&) noexcept;
        ScreenshotTab& operator=(const ScreenshotTab&) = delete;
        ScreenshotTab& operator=(ScreenshotTab&&) noexcept;
        ~ScreenshotTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
