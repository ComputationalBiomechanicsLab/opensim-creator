#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc
{
    class PerfPanel final : public IPanel {
    public:
        explicit PerfPanel(std::string_view panel_name);
        PerfPanel(const PerfPanel&) = delete;
        PerfPanel(PerfPanel&&) noexcept;
        PerfPanel& operator=(const PerfPanel&) = delete;
        PerfPanel& operator=(PerfPanel&&) noexcept;
        ~PerfPanel() noexcept;

    private:
        CStringView impl_get_name() const final;
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
