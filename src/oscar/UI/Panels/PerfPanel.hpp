#pragma once

#include <oscar/UI/Panels/IPanel.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <memory>
#include <string_view>

namespace osc
{
    class PerfPanel final : public IPanel {
    public:
        explicit PerfPanel(std::string_view panelName);
        PerfPanel(PerfPanel const&) = delete;
        PerfPanel(PerfPanel&&) noexcept;
        PerfPanel& operator=(PerfPanel const&) = delete;
        PerfPanel& operator=(PerfPanel&&) noexcept;
        ~PerfPanel() noexcept;

    private:
        CStringView implGetName() const final;
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
