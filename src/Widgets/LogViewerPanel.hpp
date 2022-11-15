#pragma once

#include "src/Widgets/VirtualPanel.hpp"

#include <memory>
#include <string_view>

namespace osc
{
    class LogViewerPanel final : public VirtualPanel {
    public:
        LogViewerPanel(std::string_view panelName);
        LogViewerPanel(LogViewerPanel const&) = delete;
        LogViewerPanel(LogViewerPanel&&) noexcept = default;
        LogViewerPanel& operator=(LogViewerPanel const&) = delete;
        LogViewerPanel& operator=(LogViewerPanel&&) noexcept = default;
        ~LogViewerPanel() noexcept;

    private:
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
