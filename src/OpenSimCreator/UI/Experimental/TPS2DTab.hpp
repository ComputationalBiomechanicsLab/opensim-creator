#pragma once

#include <oscar/UI/Tabs/Tab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class TabHost; }

namespace osc
{
    class TPS2DTab final : public Tab {
    public:
        static CStringView id();

        explicit TPS2DTab(ParentPtr<TabHost> const&);
        TPS2DTab(TPS2DTab const&) = delete;
        TPS2DTab(TPS2DTab&&) noexcept;
        TPS2DTab& operator=(TPS2DTab const&) = delete;
        TPS2DTab& operator=(TPS2DTab&&) noexcept;
        ~TPS2DTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
