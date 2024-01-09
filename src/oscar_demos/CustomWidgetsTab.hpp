#pragma once

#include <oscar/UI/Tabs/ITab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class CustomWidgetsTab final : public ITab {
    public:
        static CStringView id();

        explicit CustomWidgetsTab(ParentPtr<ITabHost> const&);
        CustomWidgetsTab(CustomWidgetsTab const&) = delete;
        CustomWidgetsTab(CustomWidgetsTab&&) noexcept;
        CustomWidgetsTab& operator=(CustomWidgetsTab const&) = delete;
        CustomWidgetsTab& operator=(CustomWidgetsTab&&) noexcept;
        ~CustomWidgetsTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
