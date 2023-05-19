#pragma once

#include <oscar/Tabs/Tab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class ModelWarpingTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit ModelWarpingTab(std::weak_ptr<TabHost>);
        ModelWarpingTab(ModelWarpingTab const&) = delete;
        ModelWarpingTab(ModelWarpingTab&&) noexcept = default;
        ModelWarpingTab& operator=(ModelWarpingTab const&) = delete;
        ModelWarpingTab& operator=(ModelWarpingTab&&) noexcept = default;
        ~ModelWarpingTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDrawMainMenu() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}