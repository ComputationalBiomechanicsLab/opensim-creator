#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <exception>
#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class ErrorTab final : public ITab {
    public:
        ErrorTab(ParentPtr<ITabHost> const&, std::exception const&);
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
