#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <exception>
#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class ErrorTab final : public Tab {
    public:
        ErrorTab(std::weak_ptr<TabHost>, std::exception const&);
        ErrorTab(ErrorTab const&) = delete;
        ErrorTab(ErrorTab&&) noexcept;
        ErrorTab& operator=(ErrorTab const&) = delete;
        ErrorTab& operator=(ErrorTab&&) noexcept;
        ~ErrorTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
