#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class TPS2DTab final : public ITab {
    public:
        static CStringView id();

        explicit TPS2DTab(ParentPtr<ITabHost> const&);
        TPS2DTab(TPS2DTab const&) = delete;
        TPS2DTab(TPS2DTab&&) noexcept;
        TPS2DTab& operator=(TPS2DTab const&) = delete;
        TPS2DTab& operator=(TPS2DTab&&) noexcept;
        ~TPS2DTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
