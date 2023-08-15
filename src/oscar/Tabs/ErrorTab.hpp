#pragma once

#include "oscar/Tabs/Tab.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <exception>
#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class TabHost; }

namespace osc
{
    class ErrorTab final : public Tab {
    public:
        ErrorTab(ParentPtr<TabHost> const&, std::exception const&);
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
