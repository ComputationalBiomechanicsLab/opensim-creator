#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class PreviewExperimentalDataTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit PreviewExperimentalDataTab(std::weak_ptr<TabHost>);
        PreviewExperimentalDataTab(PreviewExperimentalDataTab const&) = delete;
        PreviewExperimentalDataTab(PreviewExperimentalDataTab&&) noexcept;
        PreviewExperimentalDataTab& operator=(PreviewExperimentalDataTab const&) = delete;
        PreviewExperimentalDataTab& operator=(PreviewExperimentalDataTab&&) noexcept;
        ~PreviewExperimentalDataTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}