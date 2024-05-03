#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc
{
    class LogViewerPanel final : public IPanel {
    public:
        explicit LogViewerPanel(std::string_view panel_name);
        LogViewerPanel(const LogViewerPanel&) = delete;
        LogViewerPanel(LogViewerPanel&&) noexcept;
        LogViewerPanel& operator=(const LogViewerPanel&) = delete;
        LogViewerPanel& operator=(LogViewerPanel&&) noexcept;
        ~LogViewerPanel() noexcept;

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
