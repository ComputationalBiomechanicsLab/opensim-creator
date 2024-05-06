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
        ErrorTab(const ParentPtr<ITabHost>&, const std::exception&);
        ErrorTab(const ErrorTab&) = delete;
        ErrorTab(ErrorTab&&) noexcept;
        ErrorTab& operator=(const ErrorTab&) = delete;
        ErrorTab& operator=(ErrorTab&&) noexcept;
        ~ErrorTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
