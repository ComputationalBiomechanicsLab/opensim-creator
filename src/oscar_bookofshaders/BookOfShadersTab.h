#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class BookOfShadersTab final : public ITab {
    public:
        static CStringView id();

        explicit BookOfShadersTab(const ParentPtr<ITabHost>&);
        BookOfShadersTab(BookOfShadersTab const&) = delete;
        BookOfShadersTab(BookOfShadersTab&&) noexcept;
        BookOfShadersTab& operator=(const BookOfShadersTab&) = delete;
        BookOfShadersTab& operator=(BookOfShadersTab&&) noexcept;
        ~BookOfShadersTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
